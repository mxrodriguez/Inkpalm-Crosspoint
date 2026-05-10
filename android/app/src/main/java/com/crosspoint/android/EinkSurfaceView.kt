package com.crosspoint.android

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.os.PowerManager
import android.util.AttributeSet
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.locks.ReentrantLock
import kotlin.math.min

/**
 * Custom SurfaceView optimized for e-ink display on InkPalm 5.
 *
 * Supports two rendering paths:
 * 1. SurfaceHolder.lockCanvas() — standard Android rendering
 * 2. Direct framebuffer write — /dev/graphics/fb0 for hardware e-ink refresh
 *
 * E-ink waveform modes:
 * - GU16 (GC16): Full refresh, high quality, no ghosting
 * - A2: Partial refresh, fast, may have ghosting
 * - DU: Fast partial, no ghosting, limited grayscale
 */
class EinkSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr), SurfaceHolder.Callback {

    companion object {
        private const val TAG = "EinkSurfaceView"
        private const val FB_DEVICE = "/dev/graphics/fb0"
        private const val SYS_EINK_DIR = "/sys/class/epd"
        private const val SYS_WAVEFORM_DIR = "/sys/firmware/devicetree/base/soc@03000000/waveform_feature"

        // E-ink waveform modes
        const val WAVEFORM_GU16 = 0   // Full refresh (GC16), high quality
        const val WAVEFORM_A2 = 1     // Partial refresh, fast
        const val WAVEFORM_DU = 2     // Fast partial, no ghosting, limited grayscale
        const val WAVEFORM_AUTO = -1  // Let the system decide
    }

    /** E-ink refresh mode */
    annotation class WaveformMode

    // Display dimensions
    private var displayWidth = 720
    private var displayHeight = 1280
    private var displayStride = 0

    // Rendering state
    private var isRendering = false
    private var lastBitmap: Bitmap? = null
    private var renderCount = 0
    private var lastFullRefresh = 0

    // Direct framebuffer support
    private var fbFile: RandomAccessFile? = null
    private var fbSupported = false
    private var fbBytesPerPixel = 4  // RGBx_8888
    private var fbWidth = 0
    private var fbHeight = 0

    // Thread safety
    private val renderLock = ReentrantLock()

    // WakeLock support
    private var wakeLock: PowerManager.WakeLock? = null
    private var wakeLockTimeout = 500L  // 300ms per port status spec + margin

    // Render callback
    var onRenderListener: OnRenderListener? = null

    interface OnRenderListener {
        fun onRenderStarted()
        fun onRenderCompleted(success: Boolean, waveformMode: Int)
    }

    init {
        holder.addCallback(this)
        isFocusable = true
        setZOrderOnTop(false)
        setZOrderMediaOverlay(true)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.i(TAG, "Surface created")
        initFramebuffer()
        acquireWakeLock()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "Surface changed: ${width}x${height}")
        displayWidth = width
        displayHeight = height
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.i(TAG, "Surface destroyed")
        releaseWakeLock()
        closeFramebuffer()
    }

    /**
     * Initialize direct framebuffer access for e-ink hardware control.
     * Falls back to standard SurfaceView rendering if /dev/graphics/fb0 is not accessible.
     */
    private fun initFramebuffer() {
        try {
            fbFile = RandomAccessFile(FB_DEVICE, "rw")

            // Try to get framebuffer info via ioctl
            // On Allwinner A133, fb0 is typically RGBx_8888
            // Read first few bytes to detect format
            fbBytesPerPixel = 4  // RGBx_8888 is standard for this device
            fbWidth = displayWidth
            fbHeight = displayHeight

            fbSupported = true
            Log.i(TAG, "Framebuffer initialized: ${fbWidth}x${fbHeight}, ${fbBytesPerPixel}BPP")
        } catch (e: Exception) {
            Log.w(TAG, "Framebuffer not available, using SurfaceHolder rendering: ${e.message}")
            fbSupported = false
        }
    }

    private fun closeFramebuffer() {
        try {
            fbFile?.close()
        } catch (e: Exception) {
            Log.w(TAG, "Error closing framebuffer: ${e.message}")
        }
        fbFile = null
        fbSupported = false
    }

    /**
     * Acquire a partial WakeLock before rendering to prevent CPU throttling.
     * Budget ARM chips throttle down during idle, causing sluggish first page turn.
     */
    private fun acquireWakeLock() {
        try {
            val pm = context.getSystemService(Context.POWER_SERVICE) as? PowerManager
            wakeLock = pm?.newWakeLock(
                PowerManager.PARTIAL_WAKE_LOCK,
                "crosspoint:render"
            )?.apply {
                setReferenceCounted(false)
            }
        } catch (e: Exception) {
            Log.w(TAG, "Could not create WakeLock: ${e.message}")
        }
    }

    private fun releaseWakeLock() {
        try {
            if (wakeLock?.isHeld == true) {
                wakeLock?.release()
            }
        } catch (e: Exception) {
            Log.w(TAG, "Error releasing WakeLock: ${e.message}")
        }
    }

    /**
     * Render a bitmap to the e-ink display.
     *
     * @param bitmap The bitmap to render
     * @param waveformMode E-ink waveform mode (GU16, A2, DU, or AUTO)
     * @param fullRefresh Force a full refresh regardless of page count
     */
    fun renderBitmap(bitmap: Bitmap, @WaveformMode waveformMode: Int = WAVEFORM_AUTO, fullRefresh: Boolean = false) {
        if (renderLock.tryLock()) {
            try {
                onRenderListener?.onRenderStarted()
                acquireWakeLock()

                val mode = when (waveformMode) {
                    WAVEFORM_AUTO -> {
                        // Use GU16 for full refresh or every N pages
                        if (fullRefresh || (renderCount % 10 == 0)) WAVEFORM_GU16 else WAVEFORM_A2
                    }
                    else -> waveformMode
                }

                isRendering = true
                lastBitmap = bitmap.copy(bitmap.config, false)
                renderCount++

                val success = when {
                    fbSupported -> renderViaFramebuffer(bitmap, mode)
                    else -> renderViaSurfaceHolder(bitmap)
                }

                if (mode == WAVEFORM_GU16) {
                    lastFullRefresh = renderCount
                }

                onRenderListener?.onRenderCompleted(success, mode)
                isRendering = false

                Log.i(TAG, "Render #$renderCount completed (mode=$mode, success=$success)")

            } finally {
                releaseWakeLock()
                renderLock.unlock()
            }
        } else {
            Log.w(TAG, "Render skipped - previous render still in progress")
        }
    }

    /**
     * Render directly to framebuffer for hardware e-ink refresh.
     * This bypasses Android's SurfaceFlinger for better e-ink control.
     */
    private fun renderViaFramebuffer(bitmap: Bitmap, waveformMode: Int): Boolean {
        return try {
            val fb = fbFile ?: return false

            // Create a scaled bitmap if needed
            val scaledBitmap = if (bitmap.width != fbWidth || bitmap.height != fbHeight) {
                Bitmap.createScaledBitmap(bitmap, fbWidth, fbHeight, true)
            } else {
                bitmap
            }

            // Convert ARGB_8888 to RGBx_8888 (4 bytes per pixel)
            val buffer = ByteBuffer.allocate(fbWidth * fbHeight * fbBytesPerPixel)
                .order(ByteOrder.nativeOrder())

            for (y in 0 until fbHeight) {
                for (x in 0 until fbWidth) {
                    val pixel = scaledBitmap.getPixel(x, y)
                    val r = (pixel shr 16) and 0xFF
                    val g = (pixel shr 8) and 0xFF
                    val b = pixel and 0xFF

                    // RGBx_8888: R, G, B, padding byte
                    buffer.put(r.toByte())
                    buffer.put(g.toByte())
                    buffer.put(b.toByte())
                    buffer.put(0x00.toByte())  // padding byte
                }
            }

            // Write to framebuffer
            fb.seek(0)
            fb.write(buffer.array())

            // Trigger e-ink refresh via ioctl (if available)
            triggerEinkRefresh(waveformMode)

            true
        } catch (e: Exception) {
            Log.e(TAG, "Framebuffer render failed: ${e.message}")
            // Fall back to SurfaceHolder
            renderViaSurfaceHolder(bitmap)
        }
    }

    /**
     * Render via standard SurfaceHolder.lockCanvas().
     * This is the fallback when direct framebuffer access is not available.
     */
    private fun renderViaSurfaceHolder(bitmap: Bitmap): Boolean {
        return try {
            val canvas: Canvas? = holder.lockCanvas()
            if (canvas != null) {
                // Clear to white
                canvas.drawColor(Color.WHITE)

                // Draw bitmap scaled to surface size
                val src = Rect(0, 0, bitmap.width, bitmap.height)
                val dst = Rect(0, 0, width, height)
                canvas.drawBitmap(bitmap, src, dst, null)

                holder.unlockCanvasAndPost(canvas)
                true
            } else {
                Log.e(TAG, "lockCanvas returned null")
                false
            }
        } catch (e: Exception) {
            Log.e(TAG, "SurfaceHolder render failed: ${e.message}")
            false
        }
    }

    /**
     * Render text content directly to the e-ink display.
     * Higher-level method that handles text layout and pagination.
     */
    fun renderTextPage(
        lines: Array<String>,
        startLine: Int,
        fontSize: Float = 28f,
        lineHeight: Int = 40,
        margin: Int = 40,
        @WaveformMode waveformMode: Int = WAVEFORM_AUTO
    ) {
        val bitmap = Bitmap.createBitmap(displayWidth, displayHeight, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.WHITE)

        val paint = Paint().apply {
            color = Color.BLACK
            textSize = fontSize
            isAntiAlias = true
        }

        val linesPerPage = (displayHeight - 2 * margin) / lineHeight
        val endLine = min(startLine + linesPerPage, lines.size)

        for (i in startLine until endLine) {
            val y = margin + (i - startLine + 1) * lineHeight
            if (i < lines.size) {
                canvas.drawText(lines[i], margin.toFloat(), y.toFloat(), paint)
            }
        }

        // Page indicator
        val pageText = "${startLine / linesPerPage + 1}/${(lines.size + linesPerPage - 1) / linesPerPage}"
        val pageX = displayWidth.toFloat() - margin.toFloat() - paint.measureText(pageText)
        val pageY = displayHeight.toFloat() - margin.toFloat() / 2f
        canvas.drawText(pageText, pageX, pageY, paint)

        renderBitmap(bitmap, waveformMode)
    }

    /**
     * Force a full e-ink refresh (GU16 mode) to clear ghosting.
     */
    fun forceFullRefresh() {
        lastBitmap?.let { bitmap ->
            renderBitmap(bitmap, WAVEFORM_GU16, fullRefresh = true)
        } ?: Log.w(TAG, "No previous bitmap to refresh with")
    }

    /**
     * Clear the display to white.
     */
    fun clearDisplay() {
        val bitmap = Bitmap.createBitmap(displayWidth, displayHeight, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.WHITE)
        renderBitmap(bitmap, WAVEFORM_GU16, fullRefresh = true)
    }

    /**
     * Attempt to trigger e-ink refresh via sysfs or ioctl.
     * This is device-specific and may not be available on all devices.
     */
    private fun triggerEinkRefresh(waveformMode: Int) {
        try {
            // Try Allwinner e-ink sysfs interface
            val epdDir = File(SYS_EINK_DIR)
            if (epdDir.exists()) {
                val modeFile = File(epdDir, "update_mode")
                if (modeFile.exists()) {
                    val modeValue = when (waveformMode) {
                        WAVEFORM_A2 -> "2"
                        WAVEFORM_DU -> "3"
                        else -> "0"  // GC16/GU16
                    }
                    modeFile.writeText(modeValue)
                    Log.d(TAG, "Set e-ink mode via sysfs: $modeValue")
                }
            }

            // Try broadcast-based refresh (Moaan ROM)
            val intent = android.content.Intent("com.moaan.inkpalm.REFRESH")
            intent.putExtra("mode", waveformMode)
            context.sendBroadcast(intent)
            Log.d(TAG, "Sent e-ink refresh broadcast: mode=$waveformMode")

        } catch (e: Exception) {
            Log.d(TAG, "E-ink refresh trigger not available: ${e.message}")
        }
    }

    /**
     * Get display information for debugging.
     */
    fun getDisplayInfo(): String {
        return buildString {
            appendLine("=== EinkSurfaceView Info ===")
            appendLine("Display: ${displayWidth}x${displayHeight}")
            appendLine("Framebuffer: ${if (fbSupported) "available ($FB_DEVICE)" else "not available"}")
            appendLine("Renders: $renderCount")
            appendLine("Last full refresh: #$lastFullRefresh")
            appendLine("Is rendering: $isRendering")
            appendLine("WakeLock: ${if (wakeLock?.isHeld == true) "held" else "not held"}")
        }
    }
}

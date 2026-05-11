package com.crosspoint.android

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.os.PowerManager
import android.util.AttributeSet
import android.util.Log
import android.view.View
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.locks.ReentrantLock
import kotlin.math.min

class EinkSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    companion object {
        private const val TAG = "EinkSurfaceView"
        private const val FB_DEVICE = "/dev/graphics/fb0"
        private const val SYS_EINK_DIR = "/sys/class/epd"

        const val WAVEFORM_GU16 = 0
        const val WAVEFORM_A2 = 1
        const val WAVEFORM_DU = 2
        const val WAVEFORM_AUTO = -1
    }

    annotation class WaveformMode

    private var displayWidth = 720
    private var displayHeight = 1280

    private val viewWidth: Int get() = if (width > 0) width else displayWidth
    private val viewHeight: Int get() = if (height > 0) height else displayHeight

    private var lastBitmap: Bitmap? = null
    private var renderCount = 0
    private var lastFullRefresh = 0

    private var fbFile: RandomAccessFile? = null
    private var fbSupported = false
    private var fbBytesPerPixel = 4
    private var fbWidth = 0
    private var fbHeight = 0

    private val renderLock = ReentrantLock()

    private var wakeLock: PowerManager.WakeLock? = null

    init {
        isFocusable = true
        initFramebuffer()
        acquireWakeLock()
    }

    private fun initFramebuffer() {
        try {
            fbFile = RandomAccessFile(FB_DEVICE, "rw")
            fbBytesPerPixel = 4
            fbWidth = displayWidth
            fbHeight = displayHeight
            fbSupported = true
            Log.i(TAG, "Framebuffer initialized: ${fbWidth}x${fbHeight}, ${fbBytesPerPixel}BPP")
        } catch (e: Exception) {
            Log.i(TAG, "Framebuffer not available: ${e.message}")
            fbSupported = false
        }
    }

    private fun closeFramebuffer() {
        try { fbFile?.close() } catch (_: Exception) {}
        fbFile = null
        fbSupported = false
    }

    private fun acquireWakeLock() {
        try {
            val pm = context.getSystemService(Context.POWER_SERVICE) as? PowerManager
            wakeLock = pm?.newWakeLock(
                PowerManager.PARTIAL_WAKE_LOCK,
                "crosspoint:render"
            )?.apply { setReferenceCounted(false) }
        } catch (_: Exception) {}
    }

    private fun releaseWakeLock() {
        try {
            if (wakeLock?.isHeld == true) wakeLock?.release()
        } catch (_: Exception) {}
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val bmp = lastBitmap
        if (bmp != null) {
            canvas.drawColor(Color.WHITE)
            val src = android.graphics.Rect(0, 0, bmp.width, bmp.height)
            val dst = android.graphics.Rect(0, 0, width, height)
            canvas.drawBitmap(bmp, src, dst, null)
        }
    }

    fun renderBitmap(bitmap: Bitmap, @WaveformMode waveformMode: Int = WAVEFORM_AUTO, fullRefresh: Boolean = false) {
        if (renderLock.tryLock()) {
            try {
                acquireWakeLock()

                val mode = when (waveformMode) {
                    WAVEFORM_AUTO -> {
                        if (fullRefresh || (renderCount % 10 == 0)) WAVEFORM_GU16 else WAVEFORM_A2
                    }
                    else -> waveformMode
                }

                lastBitmap = bitmap.copy(bitmap.config, false)
                renderCount++

                if (fbSupported) {
                    renderViaFramebuffer(bitmap, mode)
                } else {
                    invalidate()
                    triggerEinkRefresh(mode)
                }

                if (mode == WAVEFORM_GU16) {
                    lastFullRefresh = renderCount
                }

                Log.i(TAG, "Render #$renderCount mode=$mode waveform=$waveformMode")

            } finally {
                releaseWakeLock()
                renderLock.unlock()
            }
        } else {
            Log.w(TAG, "Render skipped - previous render in progress")
        }
    }

    private fun renderViaFramebuffer(bitmap: Bitmap, waveformMode: Int) {
        try {
            val fb = fbFile ?: return

            val scaledBitmap = if (bitmap.width != fbWidth || bitmap.height != fbHeight) {
                Bitmap.createScaledBitmap(bitmap, fbWidth, fbHeight, true)
            } else {
                bitmap
            }

            val buffer = ByteBuffer.allocate(fbWidth * fbHeight * fbBytesPerPixel)
                .order(ByteOrder.nativeOrder())

            for (y in 0 until fbHeight) {
                for (x in 0 until fbWidth) {
                    val pixel = scaledBitmap.getPixel(x, y)
                    val r = (pixel shr 16) and 0xFF
                    val g = (pixel shr 8) and 0xFF
                    val b = pixel and 0xFF
                    buffer.put(r.toByte())
                    buffer.put(g.toByte())
                    buffer.put(b.toByte())
                    buffer.put(0x00.toByte())
                }
            }

            fb.seek(0)
            fb.write(buffer.array())
            triggerEinkRefresh(waveformMode)
        } catch (e: Exception) {
            Log.e(TAG, "Framebuffer render failed: ${e.message}")
            invalidate()
            triggerEinkRefresh(waveformMode)
        }
    }

    fun renderTextPage(
        lines: Array<String>,
        startLine: Int,
        fontSize: Float = 28f,
        lineHeight: Int = 40,
        margin: Int = 40,
        @WaveformMode waveformMode: Int = WAVEFORM_AUTO
    ) {
        val w = viewWidth
        val h = viewHeight
        val bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.WHITE)

        val paint = Paint().apply {
            color = Color.BLACK
            textSize = fontSize
            isAntiAlias = true
        }

        val linesPerPage = (h - 2 * margin) / lineHeight
        val endLine = min(startLine + linesPerPage, lines.size)

        for (i in startLine until endLine) {
            val y = margin + (i - startLine + 1) * lineHeight
            if (i < lines.size) {
                canvas.drawText(lines[i], margin.toFloat(), y.toFloat(), paint)
            }
        }

        val pageText = "${startLine / linesPerPage + 1}/${(lines.size + linesPerPage - 1) / linesPerPage}"
        val pageX = w.toFloat() - margin.toFloat() - paint.measureText(pageText)
        val pageY = h.toFloat() - margin.toFloat() / 2f
        canvas.drawText(pageText, pageX, pageY, paint)

        renderBitmap(bitmap, waveformMode)
    }

    fun forceFullRefresh() {
        lastBitmap?.let { bitmap ->
            renderBitmap(bitmap, WAVEFORM_GU16, fullRefresh = true)
        } ?: Log.w(TAG, "No previous bitmap to refresh with")
    }

    fun clearDisplay() {
        val bitmap = Bitmap.createBitmap(viewWidth, viewHeight, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        canvas.drawColor(Color.WHITE)
        renderBitmap(bitmap, WAVEFORM_GU16, fullRefresh = true)
    }

    private fun triggerEinkRefresh(waveformMode: Int) {
        try {
            val epdDir = File(SYS_EINK_DIR)
            if (epdDir.exists()) {
                val modeFile = File(epdDir, "update_mode")
                if (modeFile.exists()) {
                    val modeValue = when (waveformMode) {
                        WAVEFORM_A2 -> "2"
                        WAVEFORM_DU -> "3"
                        else -> "0"
                    }
                    modeFile.writeText(modeValue)
                }
            }

            val intent = android.content.Intent("com.moaan.inkpalm.REFRESH")
            intent.putExtra("mode", waveformMode)
            context.sendBroadcast(intent)

        } catch (_: Exception) {}
    }

    fun getDisplayInfo(): String {
        return buildString {
            appendLine("=== EinkSurfaceView Info ===")
            appendLine("Display: ${displayWidth}x${displayHeight}")
            appendLine("Framebuffer: ${if (fbSupported) "available ($FB_DEVICE)" else "not available"}")
            appendLine("Renders: $renderCount")
            appendLine("Last full refresh: #$lastFullRefresh")
            appendLine("WakeLock: ${if (wakeLock?.isHeld == true) "held" else "not held"}")
        }
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        closeFramebuffer()
        releaseWakeLock()
    }
}

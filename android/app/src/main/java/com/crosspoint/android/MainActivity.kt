package com.crosspoint.android

import android.app.Activity
import android.content.Intent
import android.content.SharedPreferences
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.Gravity
import android.view.KeyEvent
import android.view.View
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.io.File

class MainActivity : Activity() {
    companion object {
        private const val TAG = "CrossPoint"
        private const val PREFS_NAME = "crosspoint_prefs"
        private const val KEY_FONT_SIZE = "font_size"
        private const val KEY_LINE_HEIGHT = "line_height"
        private const val KEY_LAST_CHAPTER = "last_chapter"
        private const val KEY_LAST_PAGE = "last_page"
        private const val KEY_EPUB_LOADED = "epub_loaded"
    }

    private lateinit var prefs: SharedPreferences

    private var messageView: TextView? = null
    private var scrollView: ScrollView? = null
    private var einkView: EinkSurfaceView? = null
    private var chapterListView: ScrollView? = null
    private var isReaderMode = false
    private var isChapterListVisible = false

    // Reader state
    private var currentChapter = 8  // First real content chapter (94 lines)
    private var currentPage = 0
    private var totalPages = 0
    private var chapterLines: Array<String>? = null
    private var chapterCount = 0
    private var epubLoaded = false

    // Display settings
    private var fontSize = 28f
    private var lineHeight = 40

    // Portrait dimensions (InkPalm 5)
    private val screenWidth = 720
    private val screenHeight = 1280
    private val screenMargin = 40

    // External native methods
    private external fun nativeGetVersion(): String
    private external fun nativeGetLibraryInfo(): String
    private external fun nativeLoadEpub(filepath: String, cacheDir: String): Boolean
    private external fun nativeGetChapterCount(): Int
    private external fun nativeGetChapterLines(chapterIndex: Int): Array<String>
    private external fun nativeGetChapterTitle(chapterIndex: Int): String

    init {
        System.loadLibrary("crosspoint-jni")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.i(TAG, "=== CrossPoint Android starting ===")

        prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
        fontSize = prefs.getFloat(KEY_FONT_SIZE, 28f)
        lineHeight = prefs.getInt(KEY_LINE_HEIGHT, 40)
        currentChapter = prefs.getInt(KEY_LAST_CHAPTER, 8)
        currentPage = prefs.getInt(KEY_LAST_PAGE, 0)

        val layout = FrameLayout(this).apply {
            setBackgroundColor(Color.WHITE)
        }

        // Info view (TextView-based)
        val infoLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.WHITE)
            setPadding(32, 32, 32, 32)
            visibility = View.VISIBLE
        }

        val headerView = TextView(this).apply {
            text = "CrossPoint Reader"
            setTextColor(Color.BLACK)
            textSize = 28f
            setPadding(0, 0, 0, 24)
            gravity = Gravity.CENTER_HORIZONTAL
        }

        scrollView = ScrollView(this).apply {
            setBackgroundColor(Color.WHITE)
        }

        messageView = TextView(this).apply {
            text = "Loading..."
            setTextColor(Color.DKGRAY)
            textSize = 14f
        }

        scrollView!!.addView(messageView)
        infoLayout.addView(headerView)
        infoLayout.addView(scrollView)

        // E-ink SurfaceView for reader
        einkView = EinkSurfaceView(this).apply {
            visibility = View.GONE
            setBackgroundColor(Color.WHITE)
        }

        // Chapter list view
        val chapterListLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.WHITE)
            setPadding(40, 20, 40, 20)
            visibility = View.GONE
        }

        val chapterHeader = TextView(this).apply {
            text = "Chapters"
            setTextColor(Color.BLACK)
            textSize = 24f
            setPadding(0, 0, 0, 16)
            gravity = Gravity.CENTER_HORIZONTAL
        }
        chapterListLayout.addView(chapterHeader)

        val chapterContentLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }

        chapterListView = ScrollView(this).apply {
            setBackgroundColor(Color.WHITE)
            addView(chapterContentLayout)
        }

        chapterListLayout.addView(chapterListView)

        layout.addView(infoLayout, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))
        layout.addView(einkView, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))
        layout.addView(chapterListLayout, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT
        ))

        setContentView(layout)

        showInfo()
    }

    private fun showInfo() {
        isReaderMode = false
        einkView?.visibility = View.GONE
        scrollView?.parent?.let { (it as View).visibility = View.VISIBLE }

        val version = try { nativeGetVersion() } catch (e: UnsatisfiedLinkError) { "NOT LOADED: ${e.message}" }
        val libInfo = try { nativeGetLibraryInfo() } catch (e: UnsatisfiedLinkError) { "NOT LOADED" }

        val info = buildString {
            appendLine("=== CrossPoint Reader ===")
            appendLine()
            appendLine("Version: $version")
            appendLine()
            appendLine("Native Libraries:")
            appendLine(libInfo)
            appendLine()
            appendLine("--- Device Info ---")
            appendLine("SDK: ${android.os.Build.VERSION.SDK_INT}")
            appendLine("Display: ${screenWidth}x${screenHeight} (portrait)")
            appendLine("E-ink: EINK_GC16_MODE")
            appendLine()
            appendLine("--- EinkSurfaceView ---")
            appendLine(einkView?.getDisplayInfo() ?: "Not initialized")
            appendLine()
            appendLine("--- Display Settings ---")
            appendLine("Font Size: ${fontSize.toInt()}pt")
            appendLine("Line Height: ${lineHeight}px")
            appendLine()
            appendLine("--- Button Map ---")
            appendLine("Back (1x): cycle info → reader → chapters → reader")
            appendLine("Back (2x quick): exit app")
            appendLine("Vol+/Vol-: adjust font size (info screen)")
            appendLine("Vol+/Vol-: page navigation (reader mode)")
            appendLine("Logo long: force full e-ink refresh")
            appendLine()
            appendLine("--- EPUB Test ---")

            if (!epubLoaded) {
                val epubDir = Environment.getExternalStorageDirectory()
                val epubFile = File(epubDir, "epub/test.epub")
                appendLine("EPUB path: ${epubFile.absolutePath}")
                appendLine("EPUB exists: ${epubFile.exists()}")
                appendLine("EPUB readable: ${epubFile.canRead()}")

                if (epubFile.exists() && epubFile.canRead()) {
                    val loaded = try {
                        nativeLoadEpub(epubFile.absolutePath, cacheDir.absolutePath)
                    } catch (e: Exception) {
                        Log.e(TAG, "Failed to load EPUB", e)
                        false
                    }
                    appendLine("Loaded: $loaded")

                    if (loaded) {
                        epubLoaded = true
                        chapterCount = try { nativeGetChapterCount() } catch (e: Exception) { 0 }
                        appendLine("Title: Golden Son")
                        appendLine("Author: Pierce Brown")
                        appendLine("Chapters: $chapterCount")

                        // Load chapter currentChapter for testing
                        try {
                            chapterLines = nativeGetChapterLines(currentChapter)
                            val lineCount = chapterLines?.size ?: 0
                            val pages = (lineCount + lineHeight - 1) / lineHeight
                            totalPages = pages
                            appendLine("Chapter $currentChapter: $lineCount lines, $pages pages")
                        } catch (e: Exception) {
                            appendLine("Chapter $currentChapter: Failed to load - ${e.message}")
                        }
                    }
                } else {
                    appendLine("EPUB not found at: ${epubFile.absolutePath}")
                    appendLine("Place test.epub in /sdcard/epub/")
                }
            } else {
                appendLine("EPUB already loaded")
                appendLine("Title: Golden Son")
                appendLine("Author: Pierce Brown")
                appendLine("Chapters: $chapterCount")
                if (chapterLines != null) {
                    val lineCount = chapterLines?.size ?: 0
                    appendLine("Chapter $currentChapter: $lineCount lines, $totalPages pages")
                }
            }

            appendLine()
            appendLine("Press Logo/Back to toggle reader mode")
        }

        runOnUiThread { messageView?.text = info }
        Log.i(TAG, info)
    }

    private fun showReader() {
        isReaderMode = true
        isChapterListVisible = false
        scrollView?.parent?.let { (it as View).visibility = View.GONE }
        chapterListView?.visibility = View.GONE
        einkView?.visibility = View.VISIBLE
        Log.i(TAG, "showReader: chapterLines=${chapterLines != null}, totalPages=$totalPages, currentChapter=$currentChapter")

        if (chapterLines == null || chapterLines!!.isEmpty()) {
            Log.i(TAG, "Chapter not loaded, loading chapter $currentChapter")
            loadChapter(currentChapter)
            Log.i(TAG, "After loadChapter: chapterLines=${chapterLines != null}, totalPages=$totalPages")
        }

        if (chapterLines != null && totalPages > 0) {
            Log.i(TAG, "Rendering page $currentPage of $totalPages")
            renderPage()
        } else {
            Log.e(TAG, "No chapter data available for rendering after load attempt")
        }
    }

    private fun showChapterList() {
        isChapterListVisible = true
        einkView?.visibility = View.GONE
        chapterListView?.visibility = View.VISIBLE
        scrollView?.parent?.let { (it as View).visibility = View.GONE }

        val chapterContentLayout = chapterListView?.getChildAt(0) as? LinearLayout
        chapterContentLayout?.removeAllViews()

        for (i in 0 until chapterCount) {
            val title = try {
                nativeGetChapterTitle(i)
            } catch (e: Exception) {
                "Chapter $i"
            }
            val displayTitle = title ?: "Chapter $i"
            val chapterBtn = TextView(this).apply {
                text = "$i. $displayTitle"
                setTextColor(Color.BLACK)
                textSize = 18f
                setPadding(20, 12, 20, 12)
                gravity = Gravity.CENTER_VERTICAL
                setBackgroundColor(if (i == currentChapter) Color.LTGRAY else Color.WHITE)
                setOnClickListener {
                    loadChapter(i)
                    showReader()
                }
            }
            chapterContentLayout?.addView(chapterBtn)
        }
    }

    private fun saveSettings() {
        prefs.edit()
            .putFloat(KEY_FONT_SIZE, fontSize)
            .putInt(KEY_LINE_HEIGHT, lineHeight)
            .putInt(KEY_LAST_CHAPTER, currentChapter)
            .putInt(KEY_LAST_PAGE, currentPage)
            .apply()
    }

    private fun adjustFontSize(delta: Float) {
        fontSize = (fontSize + delta).coerceIn(16f, 48f)
        lineHeight = (fontSize * 1.4f).toInt()
        saveSettings()
        if (isReaderMode && chapterLines != null) {
            currentPage = 0
            totalPages = (chapterLines!!.size + lineHeight - 1) / lineHeight
            renderPage()
        }
        Log.i(TAG, "Font size: $fontSize, Line height: $lineHeight")
    }

    private fun loadChapter(index: Int) {
        if (index < 0 || index >= chapterCount) return
        currentChapter = index
        currentPage = 0
        try {
            chapterLines = nativeGetChapterLines(currentChapter)
            val lineCount = chapterLines?.size ?: 0
            totalPages = (lineCount + lineHeight - 1) / lineHeight
            Log.i(TAG, "Loaded chapter $currentChapter: $lineCount lines, $totalPages pages")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load chapter $index", e)
        }
    }

    private fun renderPage() {
        if (chapterLines == null || einkView == null) return

        val lines = chapterLines!!
        val linesPerPage = (screenHeight - 2 * screenMargin) / lineHeight
        val startLine = currentPage * linesPerPage
        val endLine = minOf(startLine + linesPerPage, lines.size)

        Log.i(TAG, "Rendering page $currentPage: lines $startLine-${endLine - 1}")

        // Use EinkSurfaceView's text rendering
        einkView?.renderTextPage(
            lines = lines,
            startLine = startLine,
            fontSize = fontSize,
            lineHeight = lineHeight,
            margin = screenMargin,
            waveformMode = if (currentPage == 0) EinkSurfaceView.WAVEFORM_GU16 else EinkSurfaceView.WAVEFORM_AUTO
        )
    }

    private var backPressTime: Long = 0

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        val scanCode = event?.scanCode ?: 0
        val action = event?.action ?: 0

        if (action == KeyEvent.ACTION_DOWN) {
            Log.i(TAG, "KeyDown: keyCode=$keyCode, scanCode=$scanCode, readerMode=$isReaderMode, chapterList=$isChapterListVisible")

            when (keyCode) {
                KeyEvent.KEYCODE_VOLUME_UP -> {
                    if (isChapterListVisible) {
                        chapterListView?.arrowScroll(View.FOCUS_UP)
                    } else if (isReaderMode && currentPage > 0) {
                        currentPage--
                        renderPage()
                        Log.i(TAG, "Previous page: $currentPage")
                    } else if (!isReaderMode) {
                        adjustFontSize(2f)
                        showInfo()
                    }
                    return true
                }
                KeyEvent.KEYCODE_VOLUME_DOWN -> {
                    if (isChapterListVisible) {
                        chapterListView?.arrowScroll(View.FOCUS_DOWN)
                    } else if (isReaderMode && currentPage < totalPages - 1) {
                        currentPage++
                        renderPage()
                        Log.i(TAG, "Next page: $currentPage")
                    } else if (!isReaderMode) {
                        adjustFontSize(-2f)
                        showInfo()
                    }
                    return true
                }
                KeyEvent.KEYCODE_BACK -> {
                    val currentTime = System.currentTimeMillis()

                    // Check for long press (hold > 1000ms)
                    if (currentTime - backPressTime > 1000) {
                        // Long press — force full e-ink refresh
                        Log.i(TAG, "Long Back press - forcing full e-ink refresh")
                        einkView?.forceFullRefresh()
                        return true
                    }

                    if (currentTime - backPressTime < 1500) {
                        Log.i(TAG, "Double Back press - exiting app")
                        saveSettings()
                        finish()
                        return true
                    }
                    backPressTime = currentTime

                    // 3-state cycle: Info → Reader → Chapter List → Info
                    if (!isReaderMode && !isChapterListVisible) {
                        showReader()
                    } else if (isReaderMode) {
                        saveSettings()
                        showChapterList()
                    } else {
                        showReader()
                    }
                    return true
                }
            }
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onDestroy() {
        super.onDestroy()
        saveSettings()
    }
}

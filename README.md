# CrossPoint for Android (Inkpalm 5 Port)

Port of the [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) ESP32-C3 e-reader firmware to run as a native Android app on the **Moaan InkPalm 5** e-ink phone.

## What This Is

CrossPoint is an open-source EPUB reader originally built for the Xteink X4 (ESP32-C3). This fork adds Android NDK stubs for the ESP32 hardware abstraction layer (HAL) so the shared EPUB/PDF/image rendering library compiles natively on Android via JNI.

The goal: bring CrossPoint's clean, performant e-ink reading experience to the InkPalm 5's 5.2" e-ink display (1280x720, Android 8.1, Allwinner A133).

### Why This Works

- CrossPoint's core logic (EPUB parsing, text layout, fonts) is cleanly separated from hardware
- KOReader already runs on InkPalm 5, proving E-ink Android reading is viable
- InkPalm 5 has 1GB RAM vs ESP32's 380KB — massive headroom for the rendering pipeline

## Current Status

**Iteration 2 — Full Native Build (in progress)**

| Milestone | Status |
|-----------|--------|
| Android project scaffolded | Done |
| NDK CMake build (arm64-v8a, armeabi-v7a, x86, x86_64) | Done |
| 33 Android stub headers + implementations | Done |
| JNI bridge (`libcrosspoint-jni.so`) | Done |
| Core C++ libs compiled (Epub, ZipFile, expat, EpdFont, GfxRenderer) | Done |
| EPUB loading, parsing, and metadata extraction | Done |
| Chapter rendering to Android Bitmap | Done |
| Android Activity with page navigation | Done |
| APK installs and runs on InkPalm 5 | Done |
| E-ink waveform refresh tuning | Pending |
| Library browsing UI (file picker, cover art) | Pending |
| Full font/hyphenation CSS rendering | Pending |

### What Works Now

- EPUB parsing (EPUB 2 and EPUB 3)
- Chapter loading and spine ordering
- Metadata extraction (title, author, TOC)
- Page rendering to Android Bitmap via JNI
- Button-based navigation (Volume Up/Down = next/prev page)
- JPEG/PNG/BMP image decoders (stub implementations)
- ZipFile, uzlib decompression, expat XML parsing all compiled natively

## Architecture

```
┌─────────────────────────────────┐
│         Android App (Kotlin)     │
│   MainActivity  →  JNI calls    │
├─────────────────────────────────┤
│        JNI Bridge (C++)         │
│   jni_bridge.cpp                │
├─────────────────────────────────┤
│      Core C++ Libraries         │
│   Epub │ ZipFile │ expat        │
│   EpdFont │ GfxRenderer         │
│   uzlib │ Utf8 │ JsonParser     │
├─────────────────────────────────┤
│    Android Stubs (HAL layer)    │
│   Arduino, HAL, FsHelpers,      │
│   Logging, WString, base64 ...  │
└─────────────────────────────────┘
```

The Android stubs (`android/app/src/main/cpp/android_stubs/`) replace ESP32-specific APIs with Android/POSIX equivalents:

| Stub | Replaces |
|------|----------|
| `Arduino.h` | Arduino core (String, millis, etc.) |
| `FsHelpers.cpp/.h` | ESP32 SPIFFS/SD file I/O → POSIX/Android storage |
| `HalStorage.h` | SD card access → `/sdcard/` paths |
| `HalDisplay.h` | E-ink driver → Android Bitmap framebuffer |
| `Logging.h/.cpp` | `ESP_LOGI` → Android logcat |
| `WString.h` | Arduino `String` class |
| `InflateReader.cpp/.h` | uzlib inflate wrapper |
| `base64.h` | Base64 encode/decode |
| `ESP.h` | ESP platform headers |
| `Print.h` | Arduino `Print` base class |

## Project Structure

```
Inkpalm-Crosspoint/
├── android/                          # Android project
│   └── app/
│       ├── build.gradle              # AGP 7.4.2, NDK r21e, CMake 3.22.1
│       └── src/main/
│           ├── cpp/
│           │   ├── CMakeLists.txt    # NDK CMake build
│           │   ├── jni_bridge.cpp    # JNI entry point
│           │   └── android_stubs/    # ESP32 → Android stubs (33 files)
│           ├── java/.../MainActivity.kt
│           └── AndroidManifest.xml
├── lib/                              # CrossPoint core libraries
│   ├── Epub/                         # EPUB parser & renderer
│   ├── GfxRenderer/                  # Graphics/rendering engine
│   ├── EpdFont/                      # Font engine
│   ├── ZipFile/                      # ZIP archive reader
│   ├── expat/                        # XML parser
│   ├── uzlib/                        # Decompression
│   ├── Utf8/                         # UTF-8 utilities
│   ├── Txt/                          # Plain text reader
│   └── Xtc/                          # XTC comic format reader
├── src/                              # Original ESP32 firmware source
├── docs/                             # Documentation
├── scripts/                          # Build/debug scripts
├── SCOPE.md                          # Project vision & scope
├── GOVERNANCE.md                     # Community principles
└── USER_GUIDE.md                     # User guide (original firmware)
```

## Build

### Prerequisites

- Android Studio (latest stable) with NDK r21e installed
- JDK 17
- ADB for device deployment
- InkPalm 5 with USB debugging enabled

### Build the APK

```sh
cd android
./gradlew assembleDebug
```

The debug APK is output to `android/app/build/outputs/apk/debug/app-debug.apk`.

### Install on Device

```sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

### Launch

```sh
adb shell am start -n com.crosspoint.android/.MainActivity
```

### View Logs

```sh
adb logcat -s CrossPoint
```

### Build Flags

The NDK build defines:

- `-DANDROID=1` — Android platform target
- `-DCROSPOINT_ANDROID=1` — CrossPoint Android fork identifier
- C++17, no RTTI, no exceptions

## Target Device

| Property | Value |
|----------|-------|
| Device | Moaan InkPalm 5 |
| Android | 8.1 (API 27) |
| SoC | Allwinner A133 |
| Display | 5.2" E-ink, 1280x720 |
| RAM | 1GB |
| GPU | Mali-400 MP, OpenGL ES 2.0 |
| Storage | ~24GB free on `/sdcard/` |
| E-ink controller | GU16 mode (SurfaceFlinger confirmed) |

### Physical Buttons

| Button | KeyCode | ScanCode | Usage |
|--------|---------|----------|-------|
| Volume Up | 24 | 115 | Next page |
| Volume Down | 25 | 114 | Previous page |
| Logo/Back (tap) | 4 | 0 | Menu |
| Logo/Back (long) | — | — | Full e-ink refresh |
| Power | — | — | Sleep/wake |

## Contributing

Contributions welcome! See the original project's [contributing docs](./docs/contributing/README.md) and [governance](GOVERNANCE.md).

### Quick Start

1. Fork this repo
2. Create a branch (`feature/your-feature`)
3. Make changes
4. Test on device with `adb install -r`
5. Submit a PR

## Roadmap

- [ ] E-ink waveform refresh optimization (Regal, A2, GU16 modes)
- [ ] Library browsing UI with cover art
- [ ] EPUB file picker via Android intent
- [ ] Full CSS rendering pipeline
- [ ] Configurable font/layout settings
- [ ] KOReader Sync integration
- [ ] WiFi book upload (web server)

## License

MIT License. See [LICENSE](LICENSE).

## Original Project

The original CrossPoint Reader firmware (PlatformIO + ESP32-C3) lives at [crosspoint-reader/crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader).

This port is **not affiliated with Moaan or any manufacturer of the InkPalm 5 hardware**.

#pragma once
// Stub for JPEGDEC.h — third-party JPEG decoder not available on Android
// On Android, JPEG decoding will be done via BitmapFactory in Java/JNI layer.
// This stub allows the C++ code to compile; actual JPEG rendering will use
// Android's native image decoder.

#include <cstdint>
#include <cstddef>

// Minimal stub — the real JPEGDEC provides a full JPEG decoder.
// The Epub library uses this for cover image generation.
// On Android, we'll decode JPEGs in Java and pass the bitmap to C++.
class JPEGDEC {
 public:
  JPEGDEC() = default;
  ~JPEGDEC() = default;
  
  // Stub methods — will not actually decode on Android
  bool open(const uint8_t* data, size_t size) { return false; }
  void close() {}
  int getWidth() const { return 0; }
  int getHeight() const { return 0; }
};

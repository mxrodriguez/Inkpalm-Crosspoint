#pragma once
// Stub for PNGdec.h — third-party PNG decoder not available on Android
// On Android, PNG decoding will be done via BitmapFactory in Java/JNI layer.

#include <cstdint>
#include <cstddef>

class PNGdec {
 public:
  PNGdec() = default;
  ~PNGdec() = default;
  
  bool open(const uint8_t* data, size_t size) { return false; }
  void close() {}
  int getWidth() const { return 0; }
  int getHeight() const { return 0; }
};

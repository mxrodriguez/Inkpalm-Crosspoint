#include "JpegToBmpConverter.h"
#include <Logging.h>

bool JpegToBmpConverter::jpegFileToBmpStream(FsFile& jpegFile, Print& bmpOut, bool crop) {
  LOG_WRN("JPG", "JPEG->BMP converter stubbed (Android BitmapFactory TBD)");
  return false;
}

bool JpegToBmpConverter::jpegFileToBmpStreamWithSize(FsFile& jpegFile, Print& bmpOut, int targetMaxWidth,
                                                     int targetMaxHeight) {
  return false;
}

bool JpegToBmpConverter::jpegFileTo1BitBmpStreamWithSize(FsFile& jpegFile, Print& bmpOut, int targetMaxWidth,
                                                         int targetMaxHeight) {
  return false;
}

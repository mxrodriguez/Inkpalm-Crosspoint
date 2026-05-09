#include "PngToBmpConverter.h"
#include <Logging.h>

bool PngToBmpConverter::pngFileToBmpStream(FsFile& pngFile, Print& bmpOut, bool crop) {
  LOG_WRN("PNG", "PNG->BMP converter stubbed (Android BitmapFactory TBD)");
  return false;
}

bool PngToBmpConverter::pngFileToBmpStreamWithSize(FsFile& pngFile, Print& bmpOut, int targetMaxWidth,
                                                   int targetMaxHeight) {
  return false;
}

bool PngToBmpConverter::pngFileTo1BitBmpStreamWithSize(FsFile& pngFile, Print& bmpOut, int targetMaxWidth,
                                                       int targetMaxHeight) {
  return false;
}

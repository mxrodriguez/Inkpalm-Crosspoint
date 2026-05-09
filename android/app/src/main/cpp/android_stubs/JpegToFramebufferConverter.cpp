#include "JpegToFramebufferConverter.h"
#include <Logging.h>

bool JpegToFramebufferConverter::getDimensionsStatic(const std::string& imagePath, ImageDimensions& out) {
  return false;
}

bool JpegToFramebufferConverter::decodeToFramebuffer(const std::string& imagePath, GfxRenderer& renderer,
                                                     const RenderConfig& config) {
  LOG_WRN("JPG", "JPEG->framebuffer converter stubbed (Android BitmapFactory TBD)");
  return false;
}

bool JpegToFramebufferConverter::supportsFormat(const std::string& extension) {
  return extension == ".jpg" || extension == ".jpeg";
}

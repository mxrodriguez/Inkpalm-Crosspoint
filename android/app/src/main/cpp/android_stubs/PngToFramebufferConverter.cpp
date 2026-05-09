#include "PngToFramebufferConverter.h"
#include <Logging.h>

bool PngToFramebufferConverter::getDimensionsStatic(const std::string& imagePath, ImageDimensions& out) {
  return false;
}

bool PngToFramebufferConverter::decodeToFramebuffer(const std::string& imagePath, GfxRenderer& renderer,
                                                    const RenderConfig& config) {
  LOG_WRN("PNG", "PNG->framebuffer converter stubbed (Android BitmapFactory TBD)");
  return false;
}

bool PngToFramebufferConverter::supportsFormat(const std::string& extension) {
  return extension == ".png";
}

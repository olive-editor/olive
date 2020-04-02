#include "color.h"

Color::Color(const char *data, const PixelFormat::Format &format)
{
  OIIO::convert_types(PixelFormat::GetOIIOTypeDesc(format),
                      data,
                      PixelFormat::GetOIIOTypeDesc(PixelFormat::PIX_FMT_RGB32F),
                      data_,
                      PixelFormat::FormatHasAlphaChannel(format) ? kRGBAChannels : kRGBChannels);

  if (!PixelFormat::FormatHasAlphaChannel(format)) {
    set_alpha(1.0f);
  }
}

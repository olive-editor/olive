#include "color.h"

Color Color::fromHsv(const float &h, const float &s, const float &v)
{
  float C = s * v;
  float X = C * (1.0f - abs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - C;
  float Rs, Gs, Bs;

  if(h >= 0.0f && h < 60.0f) {
    Rs = C;
    Gs = X;
    Bs = 0.0f;
  }
  else if(h >= 60.0f && h < 120.0f) {
    Rs = X;
    Gs = C;
    Bs = 0.0f;
  }
  else if(h >= 120.0f && h < 180.0f) {
    Rs = 0.0f;
    Gs = C;
    Bs = X;
  }
  else if(h >= 180.0f && h < 240.0f) {
    Rs = 0.0f;
    Gs = X;
    Bs = C;
  }
  else if(h >= 240.0f && h < 300.0f) {
    Rs = X;
    Gs = 0.0f;
    Bs = C;
  }
  else {
    Rs = C;
    Gs = 0.0f;
    Bs = X;
  }

  return Color(Rs + m, Gs + m, Bs + m);
}

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

void Color::toHsv(float *hue, float *sat, float *val) const
{
  float fCMax = qMax(qMax(red(), green()), blue());
  float fCMin = qMin(qMin(red(), green()), blue());
  float fDelta = fCMax - fCMin;

  if(fDelta > 0) {
    if(fCMax == red()) {
      *hue = 60 * (fmod(((green() - blue()) / fDelta), 6));
    } else if(fCMax == green()) {
      *hue = 60 * (((blue() - red()) / fDelta) + 2);
    } else if(fCMax == blue()) {
      *hue = 60 * (((red() - green()) / fDelta) + 4);
    }

    if(fCMax > 0) {
      *sat = fDelta / fCMax;
    } else {
      *sat = 0;
    }

    *val = fCMax;
  } else {
    *hue = 0;
    *sat = 0;
    *val = fCMax;
  }

  if(*hue < 0) {
    *hue = 360 + *hue;
  }
}

float Color::hue() const
{
  float h, s, v;
  toHsv(&h, &s, &v);
  return h;
}

float Color::saturation() const
{
  float h, s, v;
  toHsv(&h, &s, &v);
  return s;
}

float Color::value() const
{
  float h, s, v;
  toHsv(&h, &s, &v);
  return v;
}

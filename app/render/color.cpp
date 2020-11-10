/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "color.h"

#include "common/clamp.h"

OLIVE_NAMESPACE_ENTER

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
  *this = fromData(data, format);
}

Color::Color(const QColor &c)
{
  set_red(c.redF());
  set_green(c.greenF());
  set_blue(c.blueF());
  set_alpha(c.alphaF());
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

float Color::hsv_hue() const
{
  float h, s, v;
  toHsv(&h, &s, &v);
  return h;
}

float Color::hsv_saturation() const
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

void Color::toHsl(float *hue, float *sat, float *lightness) const
{
  float fCMin = qMin(red(), qMin(green(), blue()));
  float fCMax = qMax(red(), qMax(green(), blue()));

  *lightness = 0.5 * (fCMin + fCMax);

  if (fCMin == fCMax)
  {
    *sat = 0;
    *hue = 0;
    return;

  }
  else if (*lightness < 0.5)
  {
    *sat = (fCMax - fCMin) / (fCMax + fCMin);
  }
  else
  {
    *sat = (fCMax - fCMin) / (2.0 - fCMax - fCMin);
  }

  if (fCMax == red())
  {
    *hue = 60 * (green() - blue()) / (fCMax - fCMin);
  }
  if (fCMax == green())
  {
    *hue = 60 * (blue() - red()) / (fCMax - fCMin) + 120;
  }
  if (fCMax == blue())
  {
    *hue = 60 * (red() - green()) / (fCMax - fCMin) + 240;
  }
  if (*hue < 0)
  {
    *hue = *hue + 360;
  }
}

float Color::hsl_hue() const
{
  float h, s, l;
  toHsl(&h, &s, &l);
  return h;
}

float Color::hsl_saturation() const
{
  float h, s, l;
  toHsl(&h, &s, &l);
  return s;
}

float Color::lightness() const
{
  float h, s, l;
  toHsl(&h, &s, &l);
  return l;
}

void Color::toData(char *data, const PixelFormat::Format &format) const
{
  OIIO::convert_types(OIIO::TypeDesc::FLOAT,
                      data_,
                      PixelFormat::GetOIIOTypeDesc(format),
                      data,
                      kRGBAChannels);
}

Color Color::fromData(const char *data, const PixelFormat::Format &format)
{
  Color c;

  OIIO::convert_types(PixelFormat::GetOIIOTypeDesc(format),
                      data,
                      OIIO::TypeDesc::FLOAT,
                      c.data_,
                      kRGBAChannels);

  return c;
}

QColor Color::toQColor() const
{
  QColor c;

  // QColor only supports values from 0.0 to 1.0 and are only used for UI representations
  c.setRedF(clamp(red(), 0.0f, 1.0f));
  c.setGreenF(clamp(green(), 0.0f, 1.0f));
  c.setBlueF(clamp(blue(), 0.0f, 1.0f));
  c.setAlphaF(clamp(alpha(), 0.0f, 1.0f));

  return c;
}

float Color::GetRoughLuminance() const
{
  return (2*red()+blue()+3*green())/6.0f;
}

const Color &Color::operator+=(const Color &rhs)
{
  for (int i=0;i<kRGBAChannels;i++) {
    data_[i] += rhs.data_[i];
  }

  return *this;
}

const Color &Color::operator-=(const Color &rhs)
{
  for (int i=0;i<kRGBAChannels;i++) {
    data_[i] -= rhs.data_[i];
  }

  return *this;
}

const Color &Color::operator*=(const float &rhs)
{
  for (int i=0;i<kRGBAChannels;i++) {
    data_[i] *= rhs;
  }

  return *this;
}

const Color &Color::operator/=(const float &rhs)
{
  for (int i=0;i<kRGBAChannels;i++) {
    data_[i] /= rhs;
  }

  return *this;
}

Color Color::operator+(const Color &rhs) const
{
  Color c(*this);
  c += rhs;
  return c;
}

Color Color::operator-(const Color &rhs) const
{
  Color c(*this);
  c -= rhs;
  return c;
}

Color Color::operator*(const float &rhs) const
{
  Color c(*this);
  c *= rhs;
  return c;
}

Color Color::operator/(const float &rhs) const
{
  Color c(*this);
  c /= rhs;
  return c;
}

OLIVE_NAMESPACE_EXIT

QDebug operator<<(QDebug debug, const OLIVE_NAMESPACE::Color &r)
{
  debug.nospace() << "[R: " << r.red() << ", G: " << r.green() << ", B: " << r.blue() << ", A: " << r.alpha() << "]";
  return debug.space();
}

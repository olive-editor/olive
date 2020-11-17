/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include <OpenImageIO/imagebuf.h>

#include "common/clamp.h"
#include "common/oiioutils.h"

OLIVE_NAMESPACE_ENTER

Color Color::fromHsv(const double &h, const double &s, const double &v)
{
  double C = s * v;
  double X = C * (1.0 - abs(fmod(h / 60.0, 2.0) - 1.0));
  double m = v - C;
  double Rs, Gs, Bs;

  if(h >= 0.0 && h < 60.0) {
    Rs = C;
    Gs = X;
    Bs = 0.0;
  }
  else if(h >= 60.0 && h < 120.0) {
    Rs = X;
    Gs = C;
    Bs = 0.0;
  }
  else if(h >= 120.0 && h < 180.0) {
    Rs = 0.0;
    Gs = C;
    Bs = X;
  }
  else if(h >= 180.0 && h < 240.0) {
    Rs = 0.0;
    Gs = X;
    Bs = C;
  }
  else if(h >= 240.0 && h < 300.0) {
    Rs = X;
    Gs = 0.0;
    Bs = C;
  }
  else {
    Rs = C;
    Gs = 0.0;
    Bs = X;
  }

  return Color(Rs + m, Gs + m, Bs + m);
}

Color::Color(const char *data, const VideoParams::Format &format, int ch_layout)
{
  *this = fromData(data, format, ch_layout);
}

Color::Color(const QColor &c)
{
  set_red(c.redF());
  set_green(c.greenF());
  set_blue(c.blueF());
  set_alpha(c.alphaF());
}

void Color::toHsv(double *hue, double *sat, double *val) const
{
  double fCMax = qMax(qMax(red(), green()), blue());
  double fCMin = qMin(qMin(red(), green()), blue());
  double fDelta = fCMax - fCMin;

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

double Color::hsv_hue() const
{
  double h, s, v;
  toHsv(&h, &s, &v);
  return h;
}

double Color::hsv_saturation() const
{
  double h, s, v;
  toHsv(&h, &s, &v);
  return s;
}

double Color::value() const
{
  double h, s, v;
  toHsv(&h, &s, &v);
  return v;
}

void Color::toHsl(double *hue, double *sat, double *lightness) const
{
  double fCMin = qMin(red(), qMin(green(), blue()));
  double fCMax = qMax(red(), qMax(green(), blue()));

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

double Color::hsl_hue() const
{
  double h, s, l;
  toHsl(&h, &s, &l);
  return h;
}

double Color::hsl_saturation() const
{
  double h, s, l;
  toHsl(&h, &s, &l);
  return s;
}

double Color::lightness() const
{
  double h, s, l;
  toHsl(&h, &s, &l);
  return l;
}

void Color::toData(char *data, const VideoParams::Format &format, int ch_layout) const
{
  OIIO::convert_pixel_values(OIIO::TypeDesc::DOUBLE,
                             data_,
                             OIIOUtils::GetOIIOBaseTypeFromFormat(format),
                             data,
                             ch_layout);
}

Color Color::fromData(const char *data, const VideoParams::Format &format, int ch_layout)
{
  Color c;

  OIIO::convert_pixel_values(OIIOUtils::GetOIIOBaseTypeFromFormat(format),
                             data,
                             OIIO::TypeDesc::DOUBLE,
                             c.data_,
                             ch_layout);

  return c;
}

QColor Color::toQColor() const
{
  QColor c;

  // QColor only supports values from 0.0 to 1.0 and are only used for UI representations
  c.setRedF(clamp(red(), 0.0, 1.0));
  c.setGreenF(clamp(green(), 0.0, 1.0));
  c.setBlueF(clamp(blue(), 0.0, 1.0));
  c.setAlphaF(clamp(alpha(), 0.0, 1.0));

  return c;
}

double Color::GetRoughLuminance() const
{
  return (2*red()+blue()+3*green())/6.0;
}

const Color &Color::operator+=(const Color &rhs)
{
  for (int i=0;i<VideoParams::kRGBAChannelCount;i++) {
    data_[i] += rhs.data_[i];
  }

  return *this;
}

const Color &Color::operator-=(const Color &rhs)
{
  for (int i=0;i<VideoParams::kRGBAChannelCount;i++) {
    data_[i] -= rhs.data_[i];
  }

  return *this;
}

const Color &Color::operator*=(const double &rhs)
{
  for (int i=0;i<VideoParams::kRGBAChannelCount;i++) {
    data_[i] *= rhs;
  }

  return *this;
}

const Color &Color::operator/=(const double &rhs)
{
  for (int i=0;i<VideoParams::kRGBAChannelCount;i++) {
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

Color Color::operator*(const double &rhs) const
{
  Color c(*this);
  c *= rhs;
  return c;
}

Color Color::operator/(const double &rhs) const
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

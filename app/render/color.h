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

#ifndef COLOR_H
#define COLOR_H

#include <QColor>
#include <QDebug>

#include "common/define.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief High precision 32-bit float based RGBA color value
 */
class Color
{
public:
  Color()
  {
    for (int i=0;i<kRGBAChannels;i++) {
      data_[i] = 0;
    }
  }

  Color(const float& r, const float& g, const float& b, const float& a = 1.0f)
  {
    data_[0] = r;
    data_[1] = g;
    data_[2] = b;
    data_[3] = a;
  }

  Color(const char *data, const PixelFormat::Format &format);

  /**
   * @brief Creates a Color struct from hue/saturation/value
   *
   * Hue expects a value between 0.0 and 360.0. Saturation and Value expect a value between 0.0 and 1.0.
   */
  static Color fromHsv(const float& h, const float& s, const float &v);

  const float& red() const {return data_[0];}
  const float& green() const {return data_[1];}
  const float& blue() const {return data_[2];}
  const float& alpha() const {return data_[3];}

  void toHsv(float* hue, float* sat, float* val) const;
  float hsv_hue() const;
  float hsv_saturation() const;
  float value() const;

  void toHsl(float* hue, float* sat, float* lightness) const;
  float hsl_hue() const;
  float hsl_saturation() const;
  float lightness() const;

  void set_red(const float& red) {data_[0] = red;}
  void set_green(const float& green) {data_[1] = green;}
  void set_blue(const float& blue) {data_[2] = blue;}
  void set_alpha(const float& alpha) {data_[3] = alpha;}

  float* data() {return data_;}
  const float* data() const {return data_;}

  static Color fromData(const char* data, const PixelFormat::Format& format);

  QColor toQColor() const;

  // Assignment math operators
  const Color& operator+=(const Color& rhs);
  const Color& operator-=(const Color& rhs);
  const Color& operator*=(const float& rhs);
  const Color& operator/=(const float& rhs);

  // Binary math operators
  Color operator+(const Color& rhs) const;
  Color operator-(const Color& rhs) const;
  Color operator*(const float& rhs) const;
  Color operator/(const float& rhs) const;

private:
  float data_[kRGBAChannels];

};

OLIVE_NAMESPACE_EXIT

QDebug operator<<(QDebug debug, const OLIVE_NAMESPACE::Color& r);

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::Color)

#endif // COLOR_H

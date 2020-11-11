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
 * @brief High precision 64-bit float based RGBA color value
 */
class Color
{
public:
  Color()
  {
    for (int i=0;i<kRGBAChannels;i++) {
      data_[i] = 0.0;
    }
  }

  Color(const double& r, const double& g, const double& b, const double& a = 1.0)
  {
    data_[0] = r;
    data_[1] = g;
    data_[2] = b;
    data_[3] = a;
  }

  Color(const char *data, const PixelFormat::Format &format);

  Color(const QColor& c);

  /**
   * @brief Creates a Color struct from hue/saturation/value
   *
   * Hue expects a value between 0.0 and 360.0. Saturation and Value expect a value between 0.0 and 1.0.
   */
  static Color fromHsv(const double& h, const double& s, const double &v);

  const double& red() const {return data_[0];}
  const double& green() const {return data_[1];}
  const double& blue() const {return data_[2];}
  const double& alpha() const {return data_[3];}

  void toHsv(double* hue, double* sat, double* val) const;
  double hsv_hue() const;
  double hsv_saturation() const;
  double value() const;

  void toHsl(double* hue, double* sat, double* lightness) const;
  double hsl_hue() const;
  double hsl_saturation() const;
  double lightness() const;

  void set_red(const double& red) {data_[0] = red;}
  void set_green(const double& green) {data_[1] = green;}
  void set_blue(const double& blue) {data_[2] = blue;}
  void set_alpha(const double& alpha) {data_[3] = alpha;}

  double* data() {return data_;}
  const double* data() const {return data_;}

  void toData(char* data, const PixelFormat::Format& format) const;

  static Color fromData(const char* data, const PixelFormat::Format& format);

  QColor toQColor() const;

  // Suuuuper rough luminance value mostly used for UI (determining whether to overlay with black
  // or white text)
  double GetRoughLuminance() const;

  // Assignment math operators
  const Color& operator+=(const Color& rhs);
  const Color& operator-=(const Color& rhs);
  const Color& operator*=(const double& rhs);
  const Color& operator/=(const double& rhs);

  // Binary math operators
  Color operator+(const Color& rhs) const;
  Color operator-(const Color& rhs) const;
  Color operator*(const double& rhs) const;
  Color operator/(const double& rhs) const;

private:
  double data_[kRGBAChannels];

};

OLIVE_NAMESPACE_EXIT

QDebug operator<<(QDebug debug, const OLIVE_NAMESPACE::Color& r);

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::Color)

#endif // COLOR_H

#ifndef COLOR_H
#define COLOR_H

#include <QColor>
#include <QDebug>

#include "common/define.h"
#include "render/pixelformat.h"

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

  /**
   * @brief Creates a Color struct from hue/saturation/value
   *
   * Hue expects a value between 0.0 and 360.0. Saturation and Value expect a value between 0.0 and 1.0.
   */
  static Color fromHsv(const float& h, const float& s, const float &v);

  Color(const char *data, const PixelFormat::Format &format);

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

private:
  float data_[kRGBAChannels];

};

QDebug operator<<(QDebug debug, const Color& r);

Q_DECLARE_METATYPE(Color)

#endif // COLOR_H

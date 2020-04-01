#ifndef COLOR_H
#define COLOR_H

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

  Color(const char *data, const PixelFormat::Format &format);

  const float& red() const {return data_[0];}
  const float& green() const {return data_[1];}
  const float& blue() const {return data_[2];}
  const float& alpha() const {return data_[3];}

  void set_red(const float& red) {data_[0] = red;}
  void set_green(const float& green) {data_[1] = green;}
  void set_blue(const float& blue) {data_[2] = blue;}
  void set_alpha(const float& alpha) {data_[3] = alpha;}

  float* data() {return data_;}
  const float* data() const {return data_;}

  static Color fromData(const char* data, const PixelFormat::Format& format);

private:
  float data_[kRGBAChannels];

};

#endif // COLOR_H

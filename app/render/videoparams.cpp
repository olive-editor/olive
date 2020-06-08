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

#include "videoparams.h"

#include <QtMath>

#include "core.h"

OLIVE_NAMESPACE_ENTER

VideoParams::VideoParams() :
  format_(PixelFormat::PIX_FMT_INVALID)
{
}

VideoParams::VideoParams(const int &width, const int &height, const PixelFormat::Format &format, const int& divider) :
  width_(width),
  height_(height),
  format_(format),
  divider_(divider)
{
  calculate_effective_size();
}

VideoParams::VideoParams(const int &width, const int &height, const rational &time_base, const PixelFormat::Format &format, const int &divider) :
  width_(width),
  height_(height),
  time_base_(time_base),
  format_(format),
  divider_(divider)
{
  calculate_effective_size();
}

int VideoParams::generate_auto_divider(qint64 width, qint64 height)
{
  // Arbitrary pixel count (from 640x360)
  const int target_res = 230400;

  qint64 megapixels = width * height;

  double squared_divider = double(megapixels) / double(target_res);
  double divider = qSqrt(squared_divider);

  QList<int> supported_dividers = Core::SupportedDividers();

  if (divider <= supported_dividers.first()) {
    return supported_dividers.first();
  } else if (divider >= supported_dividers.last()) {
    return supported_dividers.last();
  } else {
    for (int i=1; i<supported_dividers.size(); i++) {
      int prev_divider = supported_dividers.at(i-1);
      int next_divider = supported_dividers.at(i);

      if (divider >= prev_divider && divider <= next_divider) {
        double prev_diff = qAbs(prev_divider - divider);
        double next_diff = qAbs(next_divider - divider);

        if (prev_diff < next_diff) {
          return prev_divider;
        } else {
          return next_divider;
        }
      }
    }

    // "Safe" fallback
    return 2;
  }
}

bool VideoParams::operator==(const VideoParams &rhs) const
{
  return width() == rhs.width()
      && height() == rhs.height()
      && time_base() == rhs.time_base()
      && format() == rhs.format()
      && divider() == rhs.divider();
}

bool VideoParams::operator!=(const VideoParams &rhs) const
{
  return !(*this == rhs);
}

void VideoParams::calculate_effective_size()
{
  // Fast rounding up to an even number
  effective_width_ = qCeil(width() / divider_ * 0.5) * 2;
  effective_height_ = qCeil(height() / divider_ * 0.5) * 2;
}

bool VideoParams::is_valid() const
{
  return (width() > 0
          && height() > 0
          && format_ != PixelFormat::PIX_FMT_INVALID
          && format_ != PixelFormat::PIX_FMT_COUNT);
}

OLIVE_NAMESPACE_EXIT

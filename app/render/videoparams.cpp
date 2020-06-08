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

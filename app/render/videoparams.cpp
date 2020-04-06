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

OLIVE_NAMESPACE_ENTER

VideoParams::VideoParams() :
  width_(0),
  height_(0)
{

}

VideoParams::VideoParams(const int &width, const int &height, const rational &time_base) :
  width_(width),
  height_(height),
  time_base_(time_base)
{
}

const int &VideoParams::width() const
{
  return width_;
}

const int &VideoParams::height() const
{
  return height_;
}

const rational &VideoParams::time_base() const
{
  return time_base_;
}

VideoRenderingParams::VideoRenderingParams() :
  format_(PixelFormat::PIX_FMT_INVALID)
{
}

VideoRenderingParams::VideoRenderingParams(const int &width, const int &height, const PixelFormat::Format &format, const int& divider) :
  VideoParams(width, height, rational()),
  format_(format),
  divider_(divider)
{
  calculate_effective_size();
}

VideoRenderingParams::VideoRenderingParams(const int &width, const int &height, const rational &time_base, const PixelFormat::Format &format, const RenderMode::Mode& mode, const int &divider) :
  VideoParams(width, height, time_base),
  format_(format),
  mode_(mode),
  divider_(divider)
{
  calculate_effective_size();
}

VideoRenderingParams::VideoRenderingParams(const VideoParams &params, const PixelFormat::Format &format, const RenderMode::Mode& mode, const int& divider) :
  VideoParams(params),
  format_(format),
  mode_(mode),
  divider_(divider)
{
  calculate_effective_size();
}

const int &VideoRenderingParams::divider() const
{
  return divider_;
}

const int& VideoRenderingParams::effective_width() const
{
  return effective_width_;
}

const int& VideoRenderingParams::effective_height() const
{
  return effective_height_;
}

const PixelFormat::Format &VideoRenderingParams::format() const
{
  return format_;
}

const RenderMode::Mode &VideoRenderingParams::mode() const
{
  return mode_;
}

bool VideoRenderingParams::operator==(const VideoRenderingParams &rhs) const
{
  return width() == rhs.width()
      && height() == rhs.height()
      && time_base() == rhs.time_base()
      && format() == rhs.format()
      && mode() == rhs.mode()
      && divider() == rhs.divider();
}

bool VideoRenderingParams::operator!=(const VideoRenderingParams &rhs) const
{
  return width() != rhs.width()
      || height() != rhs.height()
      || time_base() != rhs.time_base()
      || format() != rhs.format()
      || mode() != rhs.mode()
      || divider() != rhs.divider();
}

void VideoRenderingParams::calculate_effective_size()
{
  effective_width_ = width() / divider_;
  effective_height_ = height() / divider_;
}

bool VideoRenderingParams::is_valid() const
{
  return (width() > 0
          && height() > 0
          && format_ != PixelFormat::PIX_FMT_INVALID
          && format_ != PixelFormat::PIX_FMT_COUNT);
}

OLIVE_NAMESPACE_EXIT

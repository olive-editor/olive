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

#include "frame.h"

#include <QDebug>
#include <QtGlobal>
#include <QtMath>

OLIVE_NAMESPACE_ENTER

Frame::Frame() :
  timestamp_(0)
{
}

FramePtr Frame::Create()
{
  return std::make_shared<Frame>();
}

const VideoParams &Frame::video_params() const
{
  return params_;
}

void Frame::set_video_params(const VideoParams &params)
{
  params_ = params;

  linesize_ = generate_linesize_bytes(width(), params_.format());
  linesize_pixels_ = linesize_ / PixelFormat::BytesPerPixel(params_.format());
}

int Frame::generate_linesize_bytes(int width, PixelFormat::Format format)
{
  // Align to 32 bytes (not sure if this is necessary?)
  return PixelFormat::BytesPerPixel(format) * ((width + 31) & ~31);
}

Color Frame::get_pixel(int x, int y) const
{
  if (!contains_pixel(x, y)) {
    return Color();
  }

  int byte_offset = y * linesize_bytes() + x * PixelFormat::BytesPerPixel(video_params().format());

  return Color(data_.data() + byte_offset, video_params().format());
}

bool Frame::contains_pixel(int x, int y) const
{
  return (is_allocated() && x >= 0 && x < width() && y >= 0 && y < height());
}

void Frame::set_pixel(int x, int y, const Color &c)
{
  if (!contains_pixel(x, y)) {
    return;
  }

  int byte_offset = y * linesize_bytes() + x * PixelFormat::BytesPerPixel(video_params().format());

  c.toData(data_.data() + byte_offset, video_params().format());
}

bool Frame::allocate()
{
  // Assume this frame is intended to be a video frame
  if (!params_.is_valid()) {
    qWarning() << "Tried to allocate a frame with invalid parameters";
    return false;
  }

  data_.resize(PixelFormat::GetBufferSize(params_.format(), linesize_, height()));

  return true;
}

OLIVE_NAMESPACE_EXIT

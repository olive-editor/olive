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

  // Align linesize to 16
  linesize_ = qCeil(static_cast<double>(width()) / 16.0) * 16;
}

int Frame::linesize_pixels() const
{
  return linesize_;
}

int Frame::linesize_bytes() const
{
  return linesize_pixels() * PixelFormat::BytesPerPixel(params_.format());
}

const int &Frame::width() const
{
  return params_.effective_width();
}

const int &Frame::height() const
{
  return params_.effective_height();
}

const PixelFormat::Format &Frame::format() const
{
  return params_.format();
}

Color Frame::get_pixel(int x, int y) const
{
  if (!contains_pixel(x, y)) {
    return Color();
  }

  int pixel_index = y * linesize_pixels() + x;

  int byte_offset = PixelFormat::GetBufferSize(video_params().format(), pixel_index, 1);

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

  int pixel_index = y * linesize_pixels() + x;

  int byte_offset = PixelFormat::GetBufferSize(video_params().format(), pixel_index, 1);

  c.toData(data_.data() + byte_offset, video_params().format());
}

const rational &Frame::timestamp() const
{
  return timestamp_;
}

void Frame::set_timestamp(const rational &timestamp)
{
  timestamp_ = timestamp;
}

char *Frame::data()
{
  return data_.data();
}

const char *Frame::const_data() const
{
  return data_.constData();
}

void Frame::allocate()
{
  // Assume this frame is intended to be a video frame
  if (!params_.is_valid()) {
    qWarning() << "Tried to allocate a frame with invalid parameters";
    return;
  }

  data_.resize(PixelFormat::GetBufferSize(params_.format(), linesize_, params_.height()));
}

bool Frame::is_allocated() const
{
  return !data_.isEmpty();
}

void Frame::destroy()
{
  data_.clear();
}

int Frame::allocated_size() const
{
  return data_.size();
}

OLIVE_NAMESPACE_EXIT

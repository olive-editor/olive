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

Frame::Frame() :
  width_(0),
  height_(0),
  format_(PixelFormat::PIX_FMT_INVALID),
  sample_count_(0),
  timestamp_(0),
  sample_aspect_ratio_(1)
{
}

FramePtr Frame::Create()
{
  return std::make_shared<Frame>();
}

const int &Frame::width() const
{
  return width_;
}

void Frame::set_width(const int &width)
{
  width_ = width;
}

const int &Frame::height() const
{
  return height_;
}

void Frame::set_height(const int &height)
{
  height_ = height;
}

const rational &Frame::sample_aspect_ratio() const
{
  return sample_aspect_ratio_;
}

void Frame::set_sample_aspect_ratio(const rational &aspect_ratio)
{
  sample_aspect_ratio_ = aspect_ratio;
}

const AudioRenderingParams &Frame::audio_params() const
{
  return audio_params_;
}

void Frame::set_audio_params(const AudioRenderingParams &params)
{
  audio_params_ = params;
}

const rational &Frame::timestamp() const
{
  return timestamp_;
}

void Frame::set_timestamp(const rational &timestamp)
{
  timestamp_ = timestamp;
}

const int64_t &Frame::native_timestamp()
{
  return native_timestamp_;
}

void Frame::set_native_timestamp(const int64_t &timestamp)
{
  native_timestamp_ = timestamp;
}

const PixelFormat::Format &Frame::format() const
{
  return format_;
}

void Frame::set_format(const PixelFormat::Format &format)
{
  format_ = format;
}

QByteArray Frame::ToByteArray() const
{
  return data_;
}

const int &Frame::sample_count() const
{
  return sample_count_;
}

void Frame::set_sample_count(const int &audio_sample_count)
{
  sample_count_ = audio_sample_count;
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
  if (width_ > 0 && height_ > 0) {
    data_.resize(PixelFormat::GetBufferSize(static_cast<PixelFormat::Format>(format_), width_, height_));
  } else if (sample_count_ > 0) {
    data_.resize(audio_params_.samples_to_bytes(sample_count_));
  }
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

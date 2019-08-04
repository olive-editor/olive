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

#include "render/pixelservice.h"

Frame::Frame() :
  width_(0),
  height_(0),
  data_(nullptr)
{
}

Frame::Frame(const Frame &f) :
  data_(nullptr)
{
  Q_UNUSED(f)
}

Frame::Frame(Frame &&f) :
  data_(f.data_)
{
  f.data_ = nullptr;
}

Frame &Frame::operator=(const Frame &f)
{
  Q_UNUSED(f)

  data_ = nullptr;
  return *this;
}

Frame &Frame::operator=(Frame &&f)
{
  if (&f != this) {
    data_ = f.data_;
    f.data_ = nullptr;
  }

  return *this;
}

Frame::~Frame()
{
  destroy();
}

const int &Frame::width()
{
  return width_;
}

void Frame::set_width(const int &width)
{
  width_ = width;
}

const int &Frame::height()
{
  return height_;
}

void Frame::set_height(const int &height)
{
  height_ = height;
}

const rational &Frame::timestamp()
{
  return timestamp_;
}

void Frame::set_timestamp(const rational &timestamp)
{
  timestamp_ = timestamp;
}

const int &Frame::format()
{
  return format_;
}

void Frame::set_format(const int &format)
{
  format_ = format;
}

uint8_t *Frame::data()
{
  return data_;
}

const uint8_t *Frame::const_data()
{
  return data_;
}

void Frame::allocate()
{
  if (data_ != nullptr) {
    destroy();
  }

  // Assume this frame is intended to be a video frame
  if (width_ > 0 && height_ > 0) {
    data_ = new uint8_t[PixelService::GetBufferSize(static_cast<olive::PixelFormat>(format_), width_, height_)];
  }

  // FIXME: Audio sample allocation
}

void Frame::destroy()
{
  delete [] data_;
  data_ = nullptr;
}

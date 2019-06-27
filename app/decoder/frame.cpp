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

#include <QtGlobal>

Frame::Frame() :
  frame_(nullptr)
{
}

Frame::Frame(AVFrame *f) :
  frame_(f)
{
}

Frame::Frame(const Frame &f) :
  frame_(nullptr)
{
  Q_UNUSED(f)
}

Frame::Frame(Frame &&f) :
  frame_(f.frame_)
{
  f.frame_ = nullptr;
}

Frame &Frame::operator=(const Frame &f)
{
  Q_UNUSED(f)

  frame_ = nullptr;
  return *this;
}

Frame &Frame::operator=(Frame &&f)
{
  if (&f != this) {
    frame_ = f.frame_;
    f.frame_ = nullptr;
  }

  return *this;
}

Frame::~Frame()
{
  FreeChild();
}

void Frame::SetAVFrame(AVFrame *f, AVRational timebase)
{
  FreeChild();

  f = frame_;
  timestamp_ = rational(timebase.num*f->pts, timebase.den);
}

const int &Frame::width()
{
  return frame_->width;
}

const int &Frame::height()
{
  return frame_->height;
}

const rational &Frame::timestamp()
{
  return timestamp_;
}

const int &Frame::format()
{
  return frame_->format;
}

uint8_t **Frame::data()
{
  return frame_->data;
}

int *Frame::linesize()
{
  return frame_->linesize;
}

void Frame::FreeChild()
{
  if (frame_ != nullptr) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }
}

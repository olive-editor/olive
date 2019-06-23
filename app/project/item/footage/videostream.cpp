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

#include "videostream.h"

VideoStream::VideoStream(Footage *footage,
                         const int &index,
                         const int &width,
                         const int &height,
                         const rational &timebase) :
  footage_(footage),
  index_(index),
  width_(width),
  height_(height),
  timebase_(timebase)
{
}

Footage *VideoStream::footage()
{
  return footage_;
}

void VideoStream::set_footage(Footage *f)
{
  footage_ = f;
}

const int &VideoStream::index()
{
  return index_;
}

void VideoStream::set_index(const int &index)
{
  index_ = index;
}

const int &VideoStream::width()
{
  return width_;
}

void VideoStream::set_width(const int &width)
{
  width_ = width;
}

const int &VideoStream::height()
{
  return height_;
}

void VideoStream::set_height(const int &height)
{
  height_ = height;
}

const rational &VideoStream::timebase()
{
  return timebase_;
}

void VideoStream::set_timebase(const rational &timebase)
{
  timebase_ = timebase;
}

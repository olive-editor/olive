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

#include "stream.h"

Stream::Stream() :
  footage_(nullptr),
  type_(kUnknown)
{

}

Stream::~Stream()
{
}

const Stream::Type &Stream::type()
{
  return type_;
}

void Stream::set_type(const Stream::Type &type)
{
  type_ = type;
}

Footage *Stream::footage()
{
  return footage_;
}

void Stream::set_footage(Footage *f)
{
  footage_ = f;
}

const rational &Stream::timebase()
{
  return timebase_;
}

void Stream::set_timebase(const rational &timebase)
{
  timebase_ = timebase;
}

const int &Stream::index()
{
  return index_;
}

void Stream::set_index(const int &index)
{
  index_ = index;
}

const int64_t &Stream::duration()
{
  return duration_;
}

void Stream::set_duration(const int64_t &duration)
{
  duration_ = duration;
}

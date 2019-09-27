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

#include "ui/icons/icons.h"

Stream::Stream() :
  footage_(nullptr),
  type_(kUnknown),
  enabled_(true)
{

}

Stream::~Stream()
{
}

QString Stream::description()
{
  return QCoreApplication::translate("Stream", "%1: Unknown").arg(index());
}

const Stream::Type &Stream::type()
{
  return type_;
}

void Stream::set_type(const Stream::Type &type)
{
  type_ = type;
}

Footage *Stream::footage() const
{
  return footage_;
}

void Stream::set_footage(Footage *f)
{
  footage_ = f;
}

const rational &Stream::timebase() const
{
  return timebase_;
}

void Stream::set_timebase(const rational &timebase)
{
  timebase_ = timebase;
}

const int &Stream::index() const
{
  return index_;
}

void Stream::set_index(const int &index)
{
  index_ = index;
}

const int64_t &Stream::duration() const
{
  return duration_;
}

void Stream::set_duration(const int64_t &duration)
{
  duration_ = duration;
}

bool Stream::enabled()
{
  return enabled_;
}

void Stream::set_enabled(bool e)
{
  enabled_ = e;
}

QIcon Stream::IconFromType(const Stream::Type &type)
{
  switch (type) {
  case Stream::kVideo:
    return olive::icon::Video;
  case Stream::kImage:
    return olive::icon::Image;
  case Stream::kAudio:
    return olive::icon::Audio;
  default:
    break;
  }

  return QIcon();
}

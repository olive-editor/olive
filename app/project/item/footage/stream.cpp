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

#include "footage.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

Stream::Stream() :
  footage_(nullptr),
  type_(kUnknown),
  enabled_(true)
{

}

Stream::~Stream()
{
}

void Stream::Load(QXmlStreamReader *reader)
{
  LoadCustomParameters(reader);
}

void Stream::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("stream");

  writer->writeAttribute("ptr", QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeAttribute("index", QString::number(index_));

  SaveCustomParameters(writer);

  writer->writeEndElement(); // stream
}

QString Stream::description() const
{
  return QCoreApplication::translate("Stream", "%1: Unknown").arg(index());
}

const Stream::Type &Stream::type() const
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
  FootageSetEvent(footage_);
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

  emit ParametersChanged();
}

bool Stream::enabled() const
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
    return icon::Video;
  case Stream::kImage:
    return icon::Image;
  case Stream::kAudio:
    return icon::Audio;
  default:
    break;
  }

  return QIcon();
}

QMutex *Stream::proxy_access_lock()
{
  return &proxy_access_lock_;
}

void Stream::FootageSetEvent(Footage*)
{
}

void Stream::LoadCustomParameters(QXmlStreamReader* reader)
{
  reader->skipCurrentElement();
}

void Stream::SaveCustomParameters(QXmlStreamWriter*) const
{
}

OLIVE_NAMESPACE_EXIT

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

Stream::Stream() :
  footage_(nullptr),
  type_(kUnknown),
  enabled_(true)
{

}

Stream::~Stream()
{
}

StreamPtr Stream::Load(QXmlStreamReader *reader, XMLNodeData &xml_node_data, const QAtomicInt* cancelled)
{
  StreamPtr stream;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("type")) {
      Stream::Type type = static_cast<Stream::Type>(attr.value().toInt());
      switch (type) {
      case Stream::kVideo:
        stream = std::make_shared<VideoStream>();
        break;
      case Stream::kAudio:
        stream = std::make_shared<AudioStream>();
        break;
      default:
        stream = std::make_shared<Stream>();
        stream->set_type(type);
        break;
      }

      // This is the only attribute we need
      break;
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("ptr")) {
      xml_node_data.footage_ptrs.insert(reader->readElementText().toULongLong(), stream);
    } else if (reader->name() == QStringLiteral("index")) {
      stream->set_index(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("timebase")) {
      stream->set_timebase(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("duration")) {
      stream->set_duration(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("enabled")) {
      stream->set_enabled(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("custom")) {
      stream->LoadCustomParameters(reader);
    } else {
      reader->skipCurrentElement();
    }
  }

  return stream;
}

void Stream::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("type"), QString::number(type_));

  writer->writeTextElement(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeTextElement(QStringLiteral("index"), QString::number(index_));

  writer->writeTextElement(QStringLiteral("timebase"), timebase_.toString());

  writer->writeTextElement(QStringLiteral("duration"), QString::number(duration_));

  writer->writeTextElement(QStringLiteral("enabled"), QString::number(enabled_));

  writer->writeStartElement(QStringLiteral("custom"));

  SaveCustomParameters(writer);

  writer->writeEndElement();
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

QIcon Stream::icon() const
{
  return QIcon();
}

void Stream::LoadCustomParameters(QXmlStreamReader* reader)
{
  reader->skipCurrentElement();
}

void Stream::SaveCustomParameters(QXmlStreamWriter*) const
{
}

}

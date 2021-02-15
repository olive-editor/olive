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

#include "common/xmlutils.h"

namespace olive {

void Stream::Load(QXmlStreamReader *reader)
{
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("type")) {
      *this = Stream(static_cast<Type>(attr.value().toInt()));
      break;
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("global")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("timebase")) {
          timebase_ = rational::fromString(reader->readElementText());
        } else if (reader->name() == QStringLiteral("duration")) {
          duration_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("channelcount")) {
          channel_count_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("enabled")) {
          enabled_ = reader->readElementText().toInt();
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("video")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("width")) {
          width_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("height")) {
          height_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("pixelaspectratio")) {
          pixel_aspect_ratio_ = rational::fromString(reader->readElementText());
        } else if (reader->name() == QStringLiteral("videotype")) {
          video_type_ = static_cast<VideoType>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("interlacing")) {
          interlacing_ = static_cast<VideoParams::Interlacing>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("pixelformat")) {
          pixel_format_ = static_cast<VideoParams::Format>(reader->readElementText().toInt());
        } else if (reader->name() == QStringLiteral("framerate")) {
          frame_rate_ = rational::fromString(reader->readElementText());
        } else if (reader->name() == QStringLiteral("starttime")) {
          start_time_ = reader->readElementText().toLongLong();
        } else if (reader->name() == QStringLiteral("premultipliedalpha")) {
          premultiplied_alpha_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("colorspace")) {
          colorspace_ = reader->readElementText();
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("audio")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("samplerate")) {
          sample_rate_ = reader->readElementText().toInt();
        } else if (reader->name() == QStringLiteral("channellayout")) {
          channel_layout_ = reader->readElementText().toULongLong();
        } else {
          reader->skipCurrentElement();
        }
      }

    } else {

      reader->skipCurrentElement();

    }
  }
}

void Stream::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("type"), QString::number(type_));

  writer->writeStartElement(QStringLiteral("global"));
    writer->writeTextElement(QStringLiteral("timebase"), timebase_.toString());
    writer->writeTextElement(QStringLiteral("duration"), QString::number(duration_));
    writer->writeTextElement(QStringLiteral("channelcount"), QString::number(channel_count_));
    writer->writeTextElement(QStringLiteral("enabled"), QString::number(enabled_));
  writer->writeEndElement(); // global

  writer->writeStartElement(QStringLiteral("video"));
    writer->writeTextElement(QStringLiteral("width"), QString::number(width_));
    writer->writeTextElement(QStringLiteral("height"), QString::number(height_));
    writer->writeTextElement(QStringLiteral("pixelaspectratio"), pixel_aspect_ratio_.toString());
    writer->writeTextElement(QStringLiteral("videotype"), QString::number(video_type_));
    writer->writeTextElement(QStringLiteral("interlacing"), QString::number(interlacing_));
    writer->writeTextElement(QStringLiteral("pixelformat"), QString::number(pixel_format_));
    writer->writeTextElement(QStringLiteral("framerate"), frame_rate_.toString());
    writer->writeTextElement(QStringLiteral("starttime"), QString::number(start_time_));
    writer->writeTextElement(QStringLiteral("premultipliedalpha"), QString::number(premultiplied_alpha_));
    writer->writeTextElement(QStringLiteral("colorspace"), colorspace_);
  writer->writeEndElement(); // video

  writer->writeStartElement(QStringLiteral("audio"));
    writer->writeTextElement(QStringLiteral("samplerate"), QString::number(sample_rate_));
    writer->writeTextElement(QStringLiteral("channellayout"), QString::number(channel_layout_));
  writer->writeEndElement(); // audio
}

}

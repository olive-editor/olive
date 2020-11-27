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

#include "audiostream.h"

#include "common/xmlutils.h"

namespace olive {

AudioStream::AudioStream()
{
  set_type(kAudio);
}

QString AudioStream::description() const
{
  return QCoreApplication::translate(
    "Stream", "%1: Audio - %n Channel(s), %2Hz", nullptr, channels())
    .arg(QString::number(index()),
         QString::number(sample_rate()));
}

const int &AudioStream::channels() const
{
  return channels_;
}

void AudioStream::set_channels(const int &channels)
{
  channels_ = channels;
}

const uint64_t &AudioStream::channel_layout() const
{
  return layout_;
}

void AudioStream::set_channel_layout(const uint64_t &layout)
{
  layout_ = layout;
}

const int &AudioStream::sample_rate() const
{
  return sample_rate_;
}

void AudioStream::set_sample_rate(const int &sample_rate)
{
  sample_rate_ = sample_rate;
}

QIcon AudioStream::icon() const
{
  return icon::Audio;
}

void AudioStream::LoadCustomParameters(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("channels")) {
      set_channels(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("layout")) {
      set_channel_layout(reader->readElementText().toULongLong());
    } else if (reader->name() == QStringLiteral("rate")) {
      set_sample_rate(reader->readElementText().toInt());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void AudioStream::SaveCustomParameters(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("channels"), QString::number(channels_));
  writer->writeTextElement(QStringLiteral("layout"), QString::number(layout_));
  writer->writeTextElement(QStringLiteral("rate"), QString::number(sample_rate_));
}

}

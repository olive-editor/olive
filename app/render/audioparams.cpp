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

#include "audioparams.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <QCoreApplication>
#include <QCryptographicHash>

#include "common/xmlutils.h"

namespace olive {

const QVector<int> AudioParams::kSupportedSampleRates = {
  8000,          // 8000 Hz
  11025,         // 11025 Hz
  16000,         // 16000 Hz
  22050,         // 22050 Hz
  24000,         // 24000 Hz
  32000,         // 32000 Hz
  44100,         // 44100 Hz
  48000,         // 48000 Hz
  88200,         // 88200 Hz
  96000          // 96000 Hz
};

const QVector<uint64_t> AudioParams::kSupportedChannelLayouts = {
  AV_CH_LAYOUT_MONO,
  AV_CH_LAYOUT_STEREO,
  AV_CH_LAYOUT_2_1,
  AV_CH_LAYOUT_5POINT1,
  AV_CH_LAYOUT_7POINT1
};

const AudioParams::Format AudioParams::kInternalFormat = AudioParams::kFormatFloat32;

qint64 AudioParams::time_to_bytes(const double &time) const
{
  Q_ASSERT(is_valid());

  return qint64(time_to_samples(time)) * channel_count() * bytes_per_sample_per_channel();
}

bool AudioParams::operator==(const AudioParams &other) const
{
  return (format() == other.format()
          && sample_rate() == other.sample_rate()
          && time_base() == other.time_base()
          && channel_layout() == other.channel_layout());
}

bool AudioParams::operator!=(const AudioParams &other) const
{
  return !(*this == other);
}

QAudioFormat::SampleType AudioParams::GetQtSampleType(AudioParams::Format format)
{
  switch (format) {
  case kFormatUnsigned8:
    return QAudioFormat::UnSignedInt;
  case kFormatSigned16:
  case kFormatSigned32:
  case kFormatSigned64:
    return QAudioFormat::SignedInt;
  case kFormatFloat32:
  case kFormatFloat64:
    return QAudioFormat::Float;
  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return QAudioFormat::Unknown;
}

qint64 AudioParams::time_to_bytes(const rational &time) const
{
  return time_to_bytes(time.toDouble());
}

qint64 AudioParams::time_to_samples(const double &time) const
{
  Q_ASSERT(is_valid());

  // NOTE: Not sure if we should round or ceil, but I've gotten better results with ceil.
  //       Specifically, we seem to occasionally get straggler ranges that never cache with round.
  return qCeil(time_base().flipped().toDouble() * time);
}

qint64 AudioParams::time_to_samples(const rational &time) const
{
  return time_to_samples(time.toDouble());
}

qint64 AudioParams::samples_to_bytes(const qint64 &samples) const
{
  Q_ASSERT(is_valid());

  return samples * channel_count() * bytes_per_sample_per_channel();
}

rational AudioParams::samples_to_time(const qint64 &samples) const
{
  return time_base() * samples;
}

qint64 AudioParams::bytes_to_samples(const qint64 &bytes) const
{
  Q_ASSERT(is_valid());

  return bytes / (channel_count() * bytes_per_sample_per_channel());
}

rational AudioParams::bytes_to_time(const qint64 &bytes) const
{
  Q_ASSERT(is_valid());

  return samples_to_time(bytes_to_samples(bytes));
}

int AudioParams::channel_count() const
{
  return av_get_channel_layout_nb_channels(channel_layout());
}

int AudioParams::bytes_per_sample_per_channel() const
{
  switch (format_) {
  case kFormatUnsigned8:
    return 1;
  case kFormatSigned16:
    return 2;
  case kFormatSigned32:
  case kFormatFloat32:
    return 4;
  case kFormatSigned64:
  case kFormatFloat64:
    return 8;
  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return 0;
}

int AudioParams::bits_per_sample() const
{
  return bytes_per_sample_per_channel() * 8;
}

bool AudioParams::is_valid() const
{
  return (!time_base().isNull()
          && channel_layout() > 0
          && format_ > kFormatInvalid
          && format_ < kFormatCount);
}

QByteArray AudioParams::toBytes() const
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  hasher.addData(reinterpret_cast<const char*>(&sample_rate_), sizeof(sample_rate_));
  hasher.addData(reinterpret_cast<const char*>(&channel_layout_), sizeof(channel_layout_));
  hasher.addData(reinterpret_cast<const char*>(&format_), sizeof(format_));
  hasher.addData(reinterpret_cast<const char*>(&timebase_), sizeof(timebase_));

  return hasher.result();
}

void AudioParams::Load(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("samplerate")) {
      set_sample_rate(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("channellayout")) {
      set_channel_layout(reader->readElementText().toULongLong());
    } else if (reader->name() == QStringLiteral("format")) {
      set_format(static_cast<AudioParams::Format>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("enabled")) {
      set_enabled(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("streamindex")) {
      set_stream_index(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("duration")) {
      set_duration(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("timebase")) {
      set_time_base(rational::fromString(reader->readElementText()));
    } else {
      reader->skipCurrentElement();
    }
  }
}

void AudioParams::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("samplerate"), QString::number(sample_rate_));
  writer->writeTextElement(QStringLiteral("channellayout"), QString::number(channel_layout_));
  writer->writeTextElement(QStringLiteral("format"), QString::number(format_));
  writer->writeTextElement(QStringLiteral("enabled"), QString::number(enabled_));
  writer->writeTextElement(QStringLiteral("streamindex"), QString::number(stream_index_));
  writer->writeTextElement(QStringLiteral("duration"), QString::number(duration_));
  writer->writeTextElement(QStringLiteral("timebase"), timebase_.toString());
}

QString AudioParams::SampleRateToString(const int &sample_rate)
{
  return QCoreApplication::translate("AudioParams", "%1 Hz").arg(sample_rate);
}

QString AudioParams::ChannelLayoutToString(const uint64_t &layout)
{
  switch (layout) {
  case AV_CH_LAYOUT_MONO:
    return QCoreApplication::translate("AudioParams", "Mono");
  case AV_CH_LAYOUT_STEREO:
    return QCoreApplication::translate("AudioParams", "Stereo");
  case AV_CH_LAYOUT_2_1:
    return QCoreApplication::translate("AudioParams", "2.1");
  case AV_CH_LAYOUT_5POINT1:
    return QCoreApplication::translate("AudioParams", "5.1");
  case AV_CH_LAYOUT_7POINT1:
    return QCoreApplication::translate("AudioParams", "7.1");
  default:
    return QCoreApplication::translate("AudioParams", "Unknown (0x%1)").arg(layout, 1, 16);
  }
}

}

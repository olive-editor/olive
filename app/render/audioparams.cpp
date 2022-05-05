/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

const AudioParams::Format AudioParams::kInternalFormat = AudioParams::kFormatFloat32Planar;

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

qint64 AudioParams::time_to_bytes(const double &time) const
{
  return time_to_bytes_per_channel(time) * channel_count();
}

qint64 AudioParams::time_to_bytes(const rational &time) const
{
  return time_to_bytes(time.toDouble());
}

qint64 AudioParams::time_to_bytes_per_channel(const double &time) const
{
  Q_ASSERT(is_valid());

  return qint64(time_to_samples(time)) * bytes_per_sample_per_channel();
}

qint64 AudioParams::time_to_bytes_per_channel(const rational &time) const
{
  return time_to_bytes_per_channel(time.toDouble());
}

qint64 AudioParams::time_to_samples(const double &time) const
{
  Q_ASSERT(is_valid());

  // NOTE: Not sure if we should round or ceil, but I've gotten better results with ceil.
  //       Specifically, we seem to occasionally get straggler ranges that never cache with round.
  return qCeil(double(sample_rate()) * time);
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
  return sample_rate_as_time_base() * samples;
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

rational AudioParams::bytes_per_channel_to_time(const qint64 &bytes) const
{
  Q_ASSERT(is_valid());

  return samples_to_time(bytes_to_samples(bytes * channel_count()));
}

int AudioParams::channel_count() const
{
  return channel_count_;
}

int AudioParams::bytes_per_sample_per_channel() const
{
  switch (format_) {
  case kFormatUnsigned8Packed:
  case kFormatUnsigned8Planar:
    return 1;
  case kFormatSigned16Packed:
  case kFormatSigned16Planar:
    return 2;
  case kFormatSigned32Packed:
  case kFormatSigned32Planar:
  case kFormatFloat32Packed:
  case kFormatFloat32Planar:
    return 4;
  case kFormatSigned64Packed:
  case kFormatSigned64Planar:
  case kFormatFloat64Packed:
  case kFormatFloat64Planar:
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

QString AudioParams::FormatToString(const Format &f)
{
  switch (f) {
  case kFormatUnsigned8Packed:
    return QCoreApplication::translate("AudioParams", "Unsigned 8-bit (Packed)");
  case kFormatSigned16Packed:
    return QCoreApplication::translate("AudioParams", "Signed 16-bit (Packed)");
  case kFormatSigned32Packed:
    return QCoreApplication::translate("AudioParams", "Signed 32-bit (Packed)");
  case kFormatSigned64Packed:
    return QCoreApplication::translate("AudioParams", "Signed 64-bit (Packed)");
  case kFormatFloat32Packed:
    return QCoreApplication::translate("AudioParams", "Float 32-bit (Packed)");
  case kFormatFloat64Packed:
    return QCoreApplication::translate("AudioParams", "Float 64-bit (Packed)");
  case kFormatUnsigned8Planar:
    return QCoreApplication::translate("AudioParams", "Unsigned 8-bit (Planar)");
  case kFormatSigned16Planar:
    return QCoreApplication::translate("AudioParams", "Signed 16-bit (Planar)");
  case kFormatSigned32Planar:
    return QCoreApplication::translate("AudioParams", "Signed 32-bit (Planar)");
  case kFormatSigned64Planar:
    return QCoreApplication::translate("AudioParams", "Signed 64-bit (Planar)");
  case kFormatFloat32Planar:
    return QCoreApplication::translate("AudioParams", "Float 32-bit (Planar)");
  case kFormatFloat64Planar:
    return QCoreApplication::translate("AudioParams", "Float 64-bit (Planar)");

  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return QCoreApplication::translate("AudioParams", "Unknown (0x%1)").arg(f, 1, 16);
}

AudioParams::Format AudioParams::GetPackedEquivalent(Format fmt)
{
  switch (fmt) {

  // For packed input, just return input
  case kFormatUnsigned8Packed:
  case kFormatSigned16Packed:
  case kFormatSigned32Packed:
  case kFormatSigned64Packed:
  case kFormatFloat32Packed:
  case kFormatFloat64Packed:
    return fmt;

  // Convert to packed
  case kFormatUnsigned8Planar:
    return kFormatUnsigned8Packed;
  case kFormatSigned16Planar:
    return kFormatSigned16Packed;
  case kFormatSigned32Planar:
    return kFormatSigned32Packed;
  case kFormatSigned64Planar:
    return kFormatSigned64Packed;
  case kFormatFloat32Planar:
    return kFormatFloat32Packed;
  case kFormatFloat64Planar:
    return kFormatFloat64Packed;

  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return kFormatInvalid;
}

AudioParams::Format AudioParams::GetPlanarEquivalent(Format fmt)
{
  switch (fmt) {

  // Convert to planar
  case kFormatUnsigned8Packed:
    return kFormatUnsigned8Planar;
  case kFormatSigned16Packed:
    return kFormatSigned16Planar;
  case kFormatSigned32Packed:
    return kFormatSigned32Planar;
  case kFormatSigned64Packed:
    return kFormatSigned64Planar;
  case kFormatFloat32Packed:
    return kFormatFloat32Planar;
  case kFormatFloat64Packed:
    return kFormatFloat64Planar;

  // For planar input, just return input
  case kFormatUnsigned8Planar:
  case kFormatSigned16Planar:
  case kFormatSigned32Planar:
  case kFormatSigned64Planar:
  case kFormatFloat32Planar:
  case kFormatFloat64Planar:
    return fmt;

  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return kFormatInvalid;
}

}

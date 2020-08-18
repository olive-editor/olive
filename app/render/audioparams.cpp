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

#include "audioparams.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <QCoreApplication>

OLIVE_NAMESPACE_ENTER

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

int AudioParams::time_to_bytes(const double &time) const
{
  Q_ASSERT(is_valid());

  return time_to_samples(time) * channel_count() * bytes_per_sample_per_channel();
}

bool AudioParams::operator==(const AudioParams &other) const
{
  return (format() == other.format()
          && sample_rate() == other.sample_rate()
          && channel_layout() == other.channel_layout());
}

bool AudioParams::operator!=(const AudioParams &other) const
{
  return !(*this == other);
}

int AudioParams::time_to_bytes(const rational &time) const
{
  return time_to_bytes(time.toDouble());
}

int AudioParams::time_to_samples(const double &time) const
{
  Q_ASSERT(is_valid());

  return qFloor(time * sample_rate());
}

int AudioParams::time_to_samples(const rational &time) const
{
  return time_to_samples(time.toDouble());
}

int AudioParams::samples_to_bytes(const int &samples) const
{
  Q_ASSERT(is_valid());

  return samples * channel_count() * bytes_per_sample_per_channel();
}

rational AudioParams::samples_to_time(const int &samples) const
{
  return rational(samples, sample_rate());
}

int AudioParams::bytes_to_samples(const int &bytes) const
{
  Q_ASSERT(is_valid());

  return bytes / (channel_count() * bytes_per_sample_per_channel());
}

rational AudioParams::bytes_to_time(const int &bytes) const
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
  case SampleFormat::SAMPLE_FMT_U8:
    return 1;
  case SampleFormat::SAMPLE_FMT_S16:
    return 2;
  case SampleFormat::SAMPLE_FMT_S32:
  case SampleFormat::SAMPLE_FMT_FLT:
    return 4;
  case SampleFormat::SAMPLE_FMT_DBL:
  case SampleFormat::SAMPLE_FMT_S64:
    return 8;
  case SampleFormat::SAMPLE_FMT_INVALID:
  case SampleFormat::SAMPLE_FMT_COUNT:
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
  return (sample_rate() > 0
          && channel_layout() > 0
          && format_ != SampleFormat::SAMPLE_FMT_INVALID
          && format_ != SampleFormat::SAMPLE_FMT_COUNT);
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

OLIVE_NAMESPACE_EXIT

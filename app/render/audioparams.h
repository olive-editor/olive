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

#ifndef AUDIOPARAMS_H
#define AUDIOPARAMS_H

#include <QAudioFormat>
#include <QtMath>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/rational.h"

namespace olive {

class AudioParams {
public:
  enum Format {
    /// Invalid
    kFormatInvalid = -1,

    /// 8-bit unsigned integer
    kFormatUnsigned8,

    /// 16-bit signed integer
    kFormatSigned16,

    /// 32-bit signed integer
    kFormatSigned32,

    /// 64-bit signed integer
    kFormatSigned64,

    /// 32-bit float
    kFormatFloat32,

    /// 64-bit float
    kFormatFloat64,

    /// Total format count
    kFormatCount
  };

  static const Format kInternalFormat;

  AudioParams() :
    sample_rate_(0),
    channel_layout_(0),
    format_(kFormatInvalid)
  {
    set_default_footage_parameters();
  }

  AudioParams(const int& sample_rate, const uint64_t& channel_layout, const Format& format) :
    sample_rate_(sample_rate),
    channel_layout_(channel_layout),
    format_(format)
  {
    set_default_footage_parameters();
  }

  int sample_rate() const
  {
    return sample_rate_;
  }

  void set_sample_rate(int sample_rate)
  {
    sample_rate_ = sample_rate;
  }

  uint64_t channel_layout() const
  {
    return channel_layout_;
  }

  void set_channel_layout(uint64_t channel_layout)
  {
    channel_layout_ = channel_layout;
  }

  rational time_base() const
  {
    if (timebase_.isNull()) {
      return rational(1, sample_rate());
    } else {
      return timebase_;
    }
  }

  void set_time_base(const rational& timebase)
  {
    timebase_ = timebase;
  }

  Format format() const
  {
    return format_;
  }

  void set_format(Format format)
  {
    format_ = format;
  }

  bool enabled() const
  {
    return enabled_;
  }

  void set_enabled(bool e)
  {
    enabled_ = e;
  }

  int stream_index() const
  {
    return stream_index_;
  }

  void set_stream_index(int s)
  {
    stream_index_ = s;
  }

  int64_t duration() const
  {
    return duration_;
  }

  void set_duration(int64_t duration)
  {
    duration_ = duration;
  }

  qint64 time_to_bytes(const double& time) const;
  qint64 time_to_bytes(const rational& time) const;
  qint64 time_to_samples(const double& time) const;
  qint64 time_to_samples(const rational& time) const;
  qint64 samples_to_bytes(const qint64& samples) const;
  rational samples_to_time(const qint64& samples) const;
  qint64 bytes_to_samples(const qint64 &bytes) const;
  rational bytes_to_time(const qint64 &bytes) const;
  int channel_count() const;
  int bytes_per_sample_per_channel() const;
  int bits_per_sample() const;
  bool is_valid() const;

  QByteArray toBytes() const;

  void Load(QXmlStreamReader* reader);

  void Save(QXmlStreamWriter* writer) const;

  bool operator==(const AudioParams& other) const;
  bool operator!=(const AudioParams& other) const;

  static QAudioFormat::SampleType GetQtSampleType(Format format);

  static const QVector<uint64_t> kSupportedChannelLayouts;
  static const QVector<int> kSupportedSampleRates;

  /**
   * @brief Convert integer sample rate to a user-friendly string
   */
  static QString SampleRateToString(const int &sample_rate);

  /**
   * @brief Convert channel layout to a user-friendly string
   */
  static QString ChannelLayoutToString(const uint64_t &layout);

private:
  void set_default_footage_parameters()
  {
    enabled_ = true;
    stream_index_ = 0;
    duration_ = 0;
  }

  int sample_rate_;

  uint64_t channel_layout_;

  Format format_;

  // Footage-specific
  bool enabled_;
  int stream_index_;
  int64_t duration_;
  rational timebase_;

};

}

Q_DECLARE_METATYPE(olive::AudioParams)

#endif // AUDIOPARAMS_H

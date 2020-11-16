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

#ifndef AUDIOPARAMS_H
#define AUDIOPARAMS_H

#include <QAudioFormat>
#include <QtMath>

#include "common/rational.h"

OLIVE_NAMESPACE_ENTER

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
  }

  AudioParams(const int& sample_rate, const uint64_t& channel_layout, const Format& format) :
    sample_rate_(sample_rate),
    channel_layout_(channel_layout),
    format_(format)
  {
  }

  const int& sample_rate() const
  {
    return sample_rate_;
  }

  const uint64_t& channel_layout() const
  {
    return channel_layout_;
  }

  rational time_base() const
  {
    return rational(1, sample_rate());
  }

  const Format &format() const
  {
    return format_;
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
  int sample_rate_;

  uint64_t channel_layout_;

  Format format_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::AudioParams)

#endif // AUDIOPARAMS_H

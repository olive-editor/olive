/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

extern "C" {
#include <libavutil/channel_layout.h>
}

#include <QtMath>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/rational.h"

namespace olive {

class AudioParams {
public:
  // Only append to this list (never insert) because indexes are used in serialized files
  enum Format {
    /// Invalid
    kFormatInvalid = -1,

    /// 8-bit unsigned integer
    kFormatUnsigned8Planar,

    /// 16-bit signed integer
    kFormatSigned16Planar,

    /// 32-bit signed integer
    kFormatSigned32Planar,

    /// 64-bit signed integer
    kFormatSigned64Planar,

    /// 32-bit float
    kFormatFloat32Planar,

    /// 64-bit float
    kFormatFloat64Planar,

    /// 8-bit unsigned integer
    kFormatUnsigned8Packed,

    /// 16-bit signed integer
    kFormatSigned16Packed,

    /// 32-bit signed integer
    kFormatSigned32Packed,

    /// 64-bit signed integer
    kFormatSigned64Packed,

    /// 32-bit float
    kFormatFloat32Packed,

    /// 64-bit float
    kFormatFloat64Packed,

    /// Total format count
    kFormatCount,

    kPlanarStart = kFormatUnsigned8Planar,
    kPackedStart = kFormatUnsigned8Packed,
    kPlanarEnd = kPackedStart,
    kPackedEnd = kFormatCount
  };

  static const Format kInternalFormat;

  AudioParams() :
    sample_rate_(0),
    channel_layout_(0),
    format_(kFormatInvalid)
  {
    set_default_footage_parameters();

    // Cache channel count
    calculate_channel_count();
  }

  AudioParams(const int& sample_rate, const uint64_t& channel_layout, const Format& format) :
    sample_rate_(sample_rate),
    channel_layout_(channel_layout),
    format_(format)
  {
    set_default_footage_parameters();
    timebase_ = sample_rate_as_time_base();

    // Cache channel count
    calculate_channel_count();
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
    calculate_channel_count();
  }

  rational time_base() const
  {
    return timebase_;
  }

  void set_time_base(const rational& timebase)
  {
    timebase_ = timebase;
  }

  rational sample_rate_as_time_base() const
  {
    return rational(1, sample_rate());
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

  static bool FormatIsPacked(Format f)
  {
    return f >= kPackedStart && f < kPackedEnd;
  }

  bool FormatIsPacked() const
  {
    return FormatIsPacked(format_);
  }

  static bool FormatIsPlanar(Format f)
  {
    return f >= kPlanarStart && f < kPlanarEnd;
  }

  bool FormatIsPlanar() const
  {
    return FormatIsPlanar(format_);
  }

  qint64 time_to_bytes(const double& time) const;
  qint64 time_to_bytes(const rational& time) const;
  qint64 time_to_bytes_per_channel(const double& time) const;
  qint64 time_to_bytes_per_channel(const rational& time) const;
  qint64 time_to_samples(const double& time) const;
  qint64 time_to_samples(const rational& time) const;
  qint64 samples_to_bytes(const qint64& samples) const;
  rational samples_to_time(const qint64& samples) const;
  qint64 bytes_to_samples(const qint64 &bytes) const;
  rational bytes_to_time(const qint64 &bytes) const;
  rational bytes_per_channel_to_time(const qint64 &bytes) const;
  int channel_count() const;
  int bytes_per_sample_per_channel() const;
  int bits_per_sample() const;
  bool is_valid() const;

  QByteArray toBytes() const;

  void Load(QXmlStreamReader* reader);

  void Save(QXmlStreamWriter* writer) const;

  bool operator==(const AudioParams& other) const;
  bool operator!=(const AudioParams& other) const;

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

  static QString FormatToString(const Format &f);

  static AudioParams::Format GetPackedEquivalent(AudioParams::Format fmt);
  static AudioParams::Format GetPlanarEquivalent(AudioParams::Format fmt);

private:
  void set_default_footage_parameters()
  {
    enabled_ = true;
    stream_index_ = 0;
    duration_ = 0;
  }

  void calculate_channel_count()
  {
    channel_count_ = av_get_channel_layout_nb_channels(channel_layout());
  }

  int sample_rate_;

  uint64_t channel_layout_;

  int channel_count_;

  Format format_;

  // Footage-specific
  int enabled_; // Switching this to int fixes GCC 11 stringop-overflow issue, I guess a byte-alignment issue?
  int stream_index_;
  int64_t duration_;
  rational timebase_;

};

}

Q_DECLARE_METATYPE(olive::AudioParams)

#endif // AUDIOPARAMS_H

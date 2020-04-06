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

#include <QtMath>

#include "audio/sampleformat.h"
#include "common/rational.h"

OLIVE_NAMESPACE_ENTER

class AudioParams
{
public:
  AudioParams();
  AudioParams(const int& sample_rate, const uint64_t& channel_layout);

  const int& sample_rate() const;
  const uint64_t& channel_layout() const;
  rational time_base() const;

private:
  int sample_rate_;

  uint64_t channel_layout_;

};

class AudioRenderingParams : public AudioParams {
public:
  AudioRenderingParams();
  AudioRenderingParams(const int& sample_rate, const uint64_t& channel_layout, const SampleFormat::Format& format);
  AudioRenderingParams(const AudioParams& params, const SampleFormat::Format& format);

  int time_to_bytes(const rational& time) const;
  int time_to_samples(const rational& time) const;
  int samples_to_bytes(const int& samples) const;
  rational samples_to_time(const int& samples) const;
  int bytes_to_samples(const int &bytes) const;
  rational bytes_to_time(const int &bytes) const;
  int channel_count() const;
  int bytes_per_sample_per_channel() const;
  int bits_per_sample() const;
  bool is_valid() const;

  const SampleFormat::Format &format() const;

  bool operator==(const AudioRenderingParams& other) const;
  bool operator!=(const AudioRenderingParams& other) const;

private:
  SampleFormat::Format format_;
};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::AudioRenderingParams)

#endif // AUDIOPARAMS_H

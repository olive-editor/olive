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

#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <memory>

#include "render/audioparams.h"

namespace olive {

/**
 * @brief A buffer of audio samples
 *
 * Audio samples in this structure are always stored in PLANAR (separated by channel). This is done to simplify audio
 * rendering code. This replaces the old system of using QByteArrays (containing packed audio) and while SampleBuffer
 * replaces many of those in the rendering/processing side of things, QByteArrays are currently still in use for
 * playback, including reading to and from the cache.
 */
class SampleBuffer
{
public:
  SampleBuffer();
  SampleBuffer(const AudioParams& audio_params, const rational& length);
  SampleBuffer(const AudioParams& audio_params, size_t samples_per_channel);

  const AudioParams& audio_params() const;
  void set_audio_params(const AudioParams& params);

  const size_t &sample_count() const { return sample_count_per_channel_; }
  void set_sample_count(const size_t &sample_count);
  void set_sample_count(const rational &length)
  {
    set_sample_count(audio_params_.time_to_samples(length));
  }

  float* data(int channel)
  {
    return data_[channel].data();
  }

  const float* data(int channel) const
  {
    return data_.at(channel).data();
  }

  std::vector<float *> to_raw_ptrs()
  {
    std::vector<float *> r(data_.size());
    for (size_t i=0; i<r.size(); i++) {
      r[i] = data_[i].data();
    }
    return r;
  }

  int channel_count() const { return data_.size(); }

  bool is_allocated() const { return !data_.empty(); }
  void allocate();
  void destroy();

  void reverse();
  void speed(double speed);
  void transform_volume(float f);
  void transform_volume_for_channel(int channel, float volume);
  void transform_volume_for_sample(size_t sample_index, float volume);
  void transform_volume_for_sample_on_channel(size_t sample_index, int channel, float volume);

  void clamp();

  void silence();
  void silence(size_t start_sample, size_t end_sample);
  void silence_bytes(size_t start_byte, size_t end_byte);

  void set(int channel, const float* data, size_t sample_offset, size_t sample_length);
  void set(int channel, const float* data, size_t sample_length)
  {
    set(channel, data, 0, sample_length);
  }

private:
  void clamp_channel(int channel);

  AudioParams audio_params_;

  size_t sample_count_per_channel_;

  std::vector< std::vector<float> > data_;

};

}

Q_DECLARE_METATYPE(olive::SampleBuffer)

#endif // SAMPLEBUFFER_H

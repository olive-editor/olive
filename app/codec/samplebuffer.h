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

#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <memory>

#include "render/audioparams.h"

namespace olive {

class SampleBuffer;
using SampleBufferPtr = std::shared_ptr<SampleBuffer>;

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

  static SampleBufferPtr Create();
  static SampleBufferPtr CreateAllocated(const AudioParams& audio_params, const rational& length);
  static SampleBufferPtr CreateAllocated(const AudioParams& audio_params, int samples_per_channel);
  static SampleBufferPtr CreateFromPackedData(const AudioParams& audio_params, const QByteArray& bytes);

  DISABLE_COPY_MOVE(SampleBuffer)

  const AudioParams& audio_params() const;
  void set_audio_params(const AudioParams& params);

  const int &sample_count() const;
  void set_sample_count(const int &sample_count);

  float* data(int channel)
  {
    return data_[channel].data();
  }

  const float* data(int channel) const
  {
    return data_.at(channel).constData();
  }

  bool is_allocated() const;
  void allocate();
  void destroy();

  void reverse();
  void speed(double speed);
  void transform_volume(float f);
  void transform_volume_for_channel(int channel, float volume);
  void transform_volume_for_sample(int sample_index, float volume);
  void transform_volume_for_sample_on_channel(int sample_index, int channel, float volume);

  void fill(const float& f);
  void fill(const float& f, int start_sample, int end_sample);

  void set(int channel, const float* data, int sample_offset, int sample_length);
  void set(int channel, const float* data, int sample_length)
  {
    set(channel, data, 0, sample_length);
  }

  QByteArray toPackedData() const;

private:
  AudioParams audio_params_;

  int sample_count_per_channel_;

  QVector< QVector<float> > data_;

};

}

Q_DECLARE_METATYPE(olive::SampleBufferPtr)

#endif // SAMPLEBUFFER_H

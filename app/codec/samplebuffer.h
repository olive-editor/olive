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

#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

#include <memory>

#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

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

  virtual ~SampleBuffer();

  static SampleBufferPtr Create();
  static SampleBufferPtr CreateAllocated(const AudioRenderingParams& audio_params, int samples_per_channel);
  static SampleBufferPtr CreateFromPackedData(const AudioRenderingParams& audio_params, const QByteArray& bytes);

  DISABLE_COPY_MOVE(SampleBuffer)

  const AudioRenderingParams& audio_params() const;
  void set_audio_params(const AudioRenderingParams& params);

  const int &sample_count_per_channel() const;
  void set_sample_count_per_channel(const int &sample_count_per_channel);

  float** data();
  const float** const_data() const;
  float* channel_data(int channel);
  float* sample_data(int index);

  bool is_allocated() const;
  void allocate();
  void destroy();

  void reverse();
  void speed(double speed);

  void fill(const float& f);
  void fill(const float& f, int start_sample, int end_sample);

  void set(const float** data, int sample_offset, int sample_length);
  void set(const float** data, int sample_length);

  QByteArray toPackedData() const;

private:
  static void allocate_sample_buffer(float*** data, int nb_channels, int nb_samples);

  static void destroy_sample_buffer(float*** data, int nb_channels);

  AudioRenderingParams audio_params_;

  int sample_count_per_channel_;

  float** data_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::SampleBufferPtr)

#endif // SAMPLEBUFFER_H

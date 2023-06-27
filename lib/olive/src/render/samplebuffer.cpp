/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "render/samplebuffer.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <string.h>

#include "util/cpuoptimize.h"
#include "util/log.h"

namespace olive {

SampleBuffer::SampleBuffer() :
  sample_count_per_channel_(0)
{
}

SampleBuffer::SampleBuffer(const AudioParams &audio_params, const rational &length) :
  audio_params_(audio_params)
{
  sample_count_per_channel_ = audio_params_.time_to_samples(length);
  allocate();
}

SampleBuffer::SampleBuffer(const AudioParams &audio_params, size_t samples_per_channel) :
  audio_params_(audio_params),
  sample_count_per_channel_(samples_per_channel)
{
  allocate();
}

SampleBuffer SampleBuffer::rip_channel(int channel) const
{
  AudioParams p = this->audio_params_;
  p.set_channel_layout(AV_CH_LAYOUT_MONO);

  SampleBuffer b(p, this->sample_count_per_channel_);
  b.fast_set(*this, 0, channel);
  return b;
}

std::vector<float> SampleBuffer::rip_channel_vector(int channel) const
{
  return data_.at(channel);
}

const AudioParams &SampleBuffer::audio_params() const
{
  return audio_params_;
}

void SampleBuffer::set_audio_params(const AudioParams &params)
{
  if (is_allocated()) {
    Log::Warning() << "Tried to set parameters on allocated sample buffer";
    return;
  }

  audio_params_ = params;
}

void SampleBuffer::set_sample_count(const size_t &sample_count)
{
  if (is_allocated()) {
    Log::Warning() << "Tried to set sample count on allocated sample buffer";
    return;
  }

  sample_count_per_channel_ = sample_count;
}

void SampleBuffer::allocate()
{
  if (!audio_params_.is_valid()) {
    Log::Warning() << "Tried to allocate sample buffer with invalid audio parameters";
    return;
  }

  if (!sample_count_per_channel_) {
    Log::Warning() << "Tried to allocate sample buffer with zero sample count";
    return;
  }

  if (is_allocated()) {
    Log::Warning() << "Tried to allocate already allocated sample buffer";
    return;
  }

  data_.resize(audio_params_.channel_count());
  for (int i=0; i<audio_params_.channel_count(); i++) {
    data_[i].resize(sample_count_per_channel_);
  }
}

void SampleBuffer::destroy()
{
  data_.clear();
}

void SampleBuffer::reverse()
{
  if (!is_allocated()) {
    Log::Warning() << "Tried to reverse an unallocated sample buffer";
    return;
  }

  size_t half_nb_sample = sample_count_per_channel_ / 2;

  for (size_t i=0;i<half_nb_sample;i++) {
    size_t opposite_ind = sample_count_per_channel_ - i - 1;

    for (int j=0;j<audio_params_.channel_count();j++) {
      std::swap(data_[j][i], data_[j][opposite_ind]);
    }
  }
}

void SampleBuffer::speed(double speed)
{
  if (!is_allocated()) {
    Log::Warning() << "Tried to speed an unallocated sample buffer";
    return;
  }

  sample_count_per_channel_ = std::llround(static_cast<double>(sample_count_per_channel_) / speed);

  std::vector< std::vector<float> > output_data;

  output_data.resize(audio_params_.channel_count());
  for (int i=0; i<audio_params_.channel_count(); i++) {
    output_data[i].resize(sample_count_per_channel_);
  }

  for (size_t i=0;i<sample_count_per_channel_;i++) {
    size_t input_index = std::floor(static_cast<double>(i) * speed);

    for (int j=0;j<audio_params_.channel_count();j++) {
      output_data[j][i] = data_[j][input_index];
    }
  }

  data_ = output_data;
}

void SampleBuffer::transform_volume(float f)
{
  transform_volume(f, this, this);
}

void SampleBuffer::transform_volume_for_channel(int channel, float volume)
{
  transform_volume_for_channel(channel, volume, this, this);
}

void SampleBuffer::transform_volume(float f, const SampleBuffer *input, SampleBuffer *output)
{
  assert(input->channel_count() == output->channel_count());
  assert(input->sample_count_per_channel_ == output->sample_count_per_channel_);

  for (int i=0;i<input->audio_params().channel_count();i++) {
    transform_volume_for_channel(i, f, input, output);
  }
}

void SampleBuffer::transform_volume_for_channel(int channel, float volume, const SampleBuffer *input, SampleBuffer *output)
{
  const float *cdat = input->data_[channel].data();
  float *odat = output->data_[channel].data();
  size_t unopt_start = 0;

  assert(input->channel_count() == output->channel_count());
  assert(input->sample_count_per_channel_ == output->sample_count_per_channel_);

#if defined(OLIVE_PROCESSOR_X86) || defined(OLIVE_PROCESSOR_ARM)
  __m128 mult = _mm_load1_ps(&volume);
  unopt_start = (input->sample_count_per_channel_ / 4) * 4;
  for (size_t j=0; j<unopt_start; j+=4) {
    const float *in_here = cdat + j;
    float *out_here = odat + j;
    __m128 samples = _mm_loadu_ps(in_here);
    __m128 multiplied = _mm_mul_ps(samples, mult);
    _mm_storeu_ps(out_here, multiplied);
  }
#endif

  for (size_t j=unopt_start; j<input->sample_count_per_channel_; j++) {
    odat[j] = cdat[j] * volume;
  }
}

void SampleBuffer::transform_volume_for_sample(size_t sample_index, float volume)
{
  for (int i=0;i<audio_params().channel_count();i++) {
    transform_volume_for_sample_on_channel(sample_index, i, volume);
  }
}

void SampleBuffer::transform_volume_for_sample_on_channel(size_t sample_index, int channel, float volume)
{
  data_[channel][sample_index] *= volume;
}

void SampleBuffer::clamp()
{
  for (int i=0; i<channel_count(); i++) {
    clamp_channel(i);
  }
}

void SampleBuffer::silence()
{
  silence(0, sample_count_per_channel_);
}

void SampleBuffer::silence(size_t start_sample, size_t end_sample)
{
  silence_bytes(start_sample * sizeof(float), end_sample * sizeof(float));
}

void SampleBuffer::silence_bytes(size_t start_byte, size_t end_byte)
{
  if (!is_allocated()) {
    Log::Warning() << "Tried to fill an unallocated sample buffer";
    return;
  }

  for (int i=0;i<audio_params().channel_count();i++) {
    memset(reinterpret_cast<char*>(data_[i].data()) + start_byte, 0, end_byte - start_byte);
  }
}

void SampleBuffer::set(int channel, const float *data, size_t sample_offset, size_t sample_length)
{
  if (!is_allocated()) {
    Log::Warning() << "Tried to fill an unallocated sample buffer";
    return;
  }

  memcpy(&data_[channel].data()[sample_offset], data, sizeof(float) * sample_length);
}

void SampleBuffer::fast_set(const SampleBuffer &other, int to, int from)
{
  if (from == -1) {
    from = to;
  }

  data_[to] = other.data_[from];
}

void SampleBuffer::clamp_channel(int channel)
{
  const float min = -1.0f;
  const float max = 1.0f;

  float *cdat = data_[channel].data();
  size_t unopt_start = 0;

#if defined(OLIVE_PROCESSOR_X86) || defined(OLIVE_PROCESSOR_ARM)
  __m128 min_sse = _mm_load1_ps(&min);
  __m128 max_sse = _mm_load1_ps(&max);

  unopt_start = (sample_count_per_channel_ / 4) * 4;
  for (size_t j=0; j<unopt_start; j+=4) {
    float *here = cdat + j;
    __m128 samples = _mm_loadu_ps(here);

    samples = _mm_max_ps(samples, min_sse);
    samples = _mm_min_ps(samples, max_sse);

    _mm_storeu_ps(here, samples);
  }
#endif

  for (size_t sample=unopt_start; sample<sample_count(); sample++) {
    float &s = data(channel)[sample];
    s = std::clamp(s, min, max);
  }
}

}

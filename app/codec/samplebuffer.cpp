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

#include "samplebuffer.h"

OLIVE_NAMESPACE_ENTER

SampleBuffer::SampleBuffer() :
  sample_count_per_channel_(0),
  data_(nullptr)
{
}

SampleBuffer::~SampleBuffer()
{
  destroy();
}

SampleBufferPtr SampleBuffer::Create()
{
  return std::make_shared<SampleBuffer>();
}

SampleBufferPtr SampleBuffer::CreateAllocated(const AudioParams &audio_params, int samples_per_channel)
{
  SampleBufferPtr buffer = Create();

  buffer->set_audio_params(audio_params);
  buffer->set_sample_count(samples_per_channel);
  buffer->allocate();

  return buffer;
}

SampleBufferPtr SampleBuffer::CreateFromPackedData(const AudioParams &audio_params, const QByteArray &bytes)
{
  if (!audio_params.is_valid()) {
    qWarning() << "Tried to create from packed data with invalid parameters";
    return nullptr;
  }

  int samples_per_channel = audio_params.bytes_to_samples(bytes.size());
  SampleBufferPtr buffer = CreateAllocated(audio_params, samples_per_channel);

  int total_samples = samples_per_channel * audio_params.channel_count();

  const float* packed_data = reinterpret_cast<const float*>(bytes.constData());

  for (int i=0;i<total_samples;i++) {
    int channel = i % audio_params.channel_count();
    int index = i / audio_params.channel_count();

    buffer->data_[channel][index] = packed_data[i];
  }

  return buffer;
}

const AudioParams &SampleBuffer::audio_params() const
{
  return audio_params_;
}

void SampleBuffer::set_audio_params(const AudioParams &params)
{
  if (data_) {
    qWarning() << "Tried to set parameters on allocated sample buffer";
    return;
  }

  audio_params_ = params;
}

const int &SampleBuffer::sample_count() const
{
  return sample_count_per_channel_;
}

void SampleBuffer::set_sample_count(const int &sample_count)
{
  if (data_) {
    qWarning() << "Tried to set sample count on allocated sample buffer";
    return;
  }

  sample_count_per_channel_ = sample_count;
}

float **SampleBuffer::data()
{
  return data_;
}

const float **SampleBuffer::const_data() const
{
  return const_cast<const float**>(data_);
}

float *SampleBuffer::channel_data(int channel)
{
  return data_[channel];
}

bool SampleBuffer::is_allocated() const
{
  return data_;
}

void SampleBuffer::allocate()
{
  if (!audio_params_.is_valid()) {
    qWarning() << "Tried to allocate sample buffer with invalid audio parameters";
    return;
  }

  if (!sample_count_per_channel_) {
    qWarning() << "Tried to allocate sample buffer with zero sample count";
    return;
  }

  if (data_) {
    qWarning() << "Tried to allocate already allocated sample buffer";
    return;
  }

  allocate_sample_buffer(&data_, audio_params_.channel_count(), sample_count_per_channel_);
}

void SampleBuffer::destroy()
{
  destroy_sample_buffer(&data_, audio_params_.channel_count());
}

void SampleBuffer::reverse()
{
  if (!is_allocated()) {
    qWarning() << "Tried to reverse an unallocated sample buffer";
    return;
  }

  int half_nb_sample = sample_count_per_channel_ / 2;

  for (int i=0;i<half_nb_sample;i++) {
    int opposite_ind = sample_count_per_channel_ - i - 1;

    for (int j=0;j<audio_params_.channel_count();j++) {
      std::swap(data_[j][i], data_[j][opposite_ind]);
    }
  }
}

void SampleBuffer::speed(double speed)
{
  if (!is_allocated()) {
    qWarning() << "Tried to speed an unallocated sample buffer";
    return;
  }

  sample_count_per_channel_ = qRound(static_cast<double>(sample_count_per_channel_) / speed);

  float** input_data = data_;
  float** output_data;

  allocate_sample_buffer(&output_data, audio_params_.channel_count(), sample_count_per_channel_);

  for (int i=0;i<sample_count_per_channel_;i++) {
    int input_index = qFloor(static_cast<double>(i) * speed);

    for (int j=0;j<audio_params_.channel_count();j++) {
      output_data[j][i] = input_data[j][input_index];
    }
  }

  destroy_sample_buffer(&input_data, audio_params_.channel_count());

  data_ = output_data;
}

void SampleBuffer::transform_volume(float f)
{
  for (int i=0;i<audio_params().channel_count();i++) {
    for (int j=0;j<sample_count_per_channel_;j++) {
      data_[i][j] *= f;
    }
  }
}

void SampleBuffer::transform_volume_for_channel(int channel, float volume)
{
  for (int i=0;i<sample_count_per_channel_;i++) {
    data_[channel][i] *= volume;
  }
}

void SampleBuffer::transform_volume_for_sample(int sample_index, float volume)
{
  for (int i=0;i<audio_params().channel_count();i++) {
    data_[i][sample_index] *= volume;
  }
}

void SampleBuffer::transform_volume_for_sample_on_channel(int sample_index, int channel, float volume)
{
  data_[channel][sample_index] *= volume;
}

void SampleBuffer::fill(const float &f)
{
  fill(f, 0, sample_count_per_channel_);
}

void SampleBuffer::fill(const float &f, int start_sample, int end_sample)
{
  if (!is_allocated()) {
    qWarning() << "Tried to fill an unallocated sample buffer";
    return;
  }

  for (int i=0;i<audio_params().channel_count();i++) {
    for (int j=start_sample;j<end_sample;j++) {
      data_[i][j] = f;
    }
  }
}

void SampleBuffer::set(const float **data, int sample_offset, int sample_length)
{
  if (!is_allocated()) {
    qWarning() << "Tried to fill an unallocated sample buffer";
    return;
  }

  for (int i=0;i<audio_params().channel_count();i++) {
    for (int j=0;j<sample_length;j++) {
      data_[i][j + sample_offset] = data[i][j];
    }
  }
}

void SampleBuffer::set(const float **data, int sample_length)
{
  set(data, 0, sample_length);
}

QByteArray SampleBuffer::toPackedData() const
{
  QByteArray packed_data;

  if (is_allocated()) {
    packed_data.resize(audio_params_.samples_to_bytes(sample_count_per_channel_));

    float* output_data = reinterpret_cast<float*>(packed_data.data());

    int output_index = 0;

    for (int j=0;j<sample_count_per_channel_;j++) {
      for (int i=0;i<audio_params_.channel_count();i++) {
        output_data[output_index] = data_[i][j];

        output_index++;
      }
    }
  }

  return packed_data;
}

void SampleBuffer::allocate_sample_buffer(float ***data, int nb_channels, int nb_samples)
{
  Q_ASSERT(nb_samples > 0);

  *data = new float* [nb_channels];

  for (int i=0;i<nb_channels;i++) {
    // Initialise all values to 0.
    (*data)[i] = new float[nb_samples]{};
  }
}

void SampleBuffer::destroy_sample_buffer(float ***data, int nb_channels)
{
  if (*data) {
    for (int i=0;i<nb_channels;i++) {
      delete [] (*data)[i];
    }

    delete [] *data;
    *data = nullptr;
  }
}

OLIVE_NAMESPACE_EXIT

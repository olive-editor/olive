#include "samplebuffer.h"

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

SampleBufferPtr SampleBuffer::CreateAllocated(const AudioRenderingParams &audio_params, int samples_per_channel)
{
  SampleBufferPtr buffer = Create();

  buffer->set_audio_params(audio_params);
  buffer->set_sample_count_per_channel(samples_per_channel);
  buffer->allocate();

  return buffer;
}

SampleBufferPtr SampleBuffer::CreateFromPackedData(const AudioRenderingParams &audio_params, const QByteArray &bytes)
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
    int channel = i%audio_params.channel_count();
    int index = i / audio_params.channel_count();

    buffer->data_[channel][index] = packed_data[i];
  }

  return buffer;
}

const AudioRenderingParams &SampleBuffer::audio_params() const
{
  return audio_params_;
}

void SampleBuffer::set_audio_params(const AudioRenderingParams &params)
{
  if (data_) {
    qWarning() << "Tried to set parameters on allocated sample buffer";
    return;
  }

  audio_params_ = params;
}

const int &SampleBuffer::sample_count_per_channel() const
{
  return sample_count_per_channel_;
}

void SampleBuffer::set_sample_count_per_channel(const int &sample_count)
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

  int max_adjusted_nb_samples = qRound(static_cast<double>(sample_count_per_channel_) * speed);

  float** input_data = data_;
  float** output_data;

  allocate_sample_buffer(&output_data, audio_params_.channel_count(), max_adjusted_nb_samples);

  for (int i=0;i<max_adjusted_nb_samples;i++) {
    int input_index = qRound(static_cast<double>(i) * speed);

    for (int j=0;j<audio_params_.channel_count();j++) {
      output_data[j][i] = input_data[j][input_index];
    }
  }

  destroy_sample_buffer(&input_data, audio_params_.channel_count());

  data_ = output_data;
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
  *data = new float* [nb_channels];

  for (int i=0;i<nb_channels;i++) {
    (*data)[i] = new float[nb_samples];
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

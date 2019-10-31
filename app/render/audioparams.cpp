#include "audioparams.h"

extern "C" {
#include <libavformat/avformat.h>
}

AudioParams::AudioParams() :
  sample_rate_(0),
  channel_layout_(0)
{
}

AudioParams::AudioParams(const int &sample_rate, const uint64_t &channel_layout) :
  sample_rate_(sample_rate),
  channel_layout_(channel_layout)
{
}

const int &AudioParams::sample_rate() const
{
  return sample_rate_;
}

const uint64_t &AudioParams::channel_layout() const
{
  return channel_layout_;
}

AudioRenderingParams::AudioRenderingParams() :
  format_(olive::SAMPLE_FMT_INVALID)
{
}

AudioRenderingParams::AudioRenderingParams(const int &sample_rate, const uint64_t &channel_layout, const olive::SampleFormat &format) :
  AudioParams(sample_rate, channel_layout),
  format_(format)
{
}

AudioRenderingParams::AudioRenderingParams(const AudioParams &params, const olive::SampleFormat &format) :
  AudioParams(params),
  format_(format)
{
}

const olive::SampleFormat &AudioRenderingParams::format() const
{
  return format_;
}

int AudioRenderingParams::time_to_bytes(const rational &time) const
{
  Q_ASSERT(is_valid());

  return qFloor(time.toDouble() * sample_rate()) * channel_count() * bytes_per_sample_per_channel();
}

int AudioRenderingParams::samples_to_bytes(const int &samples) const
{
  Q_ASSERT(is_valid());

  return samples * channel_count() * bytes_per_sample_per_channel();
}

int AudioRenderingParams::channel_count() const
{
  return av_get_channel_layout_nb_channels(channel_layout());
}

int AudioRenderingParams::bytes_per_sample_per_channel() const
{
  switch (format_) {
  case olive::SAMPLE_FMT_U8:
    return 1;
  case olive::SAMPLE_FMT_S16:
    return 2;
  case olive::SAMPLE_FMT_S32:
  case olive::SAMPLE_FMT_FLT:
    return 4;
  case olive::SAMPLE_FMT_DBL:
  case olive::SAMPLE_FMT_S64:
    return 8;
  case olive::SAMPLE_FMT_INVALID:
  case olive::SAMPLE_FMT_COUNT:
    break;
  }

  return 0;
}

int AudioRenderingParams::bits_per_sample() const
{
  return bytes_per_sample_per_channel() * 8;
}

bool AudioRenderingParams::is_valid() const
{
  return (sample_rate() > 0
          && channel_layout() > 0
          && format_ != olive::SAMPLE_FMT_INVALID
          && format_ != olive::SAMPLE_FMT_COUNT);
}

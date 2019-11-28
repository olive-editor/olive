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
  format_(SAMPLE_FMT_INVALID)
{
}

AudioRenderingParams::AudioRenderingParams(const int &sample_rate, const uint64_t &channel_layout, const SampleFormat &format) :
  AudioParams(sample_rate, channel_layout),
  format_(format)
{
}

AudioRenderingParams::AudioRenderingParams(const AudioParams &params, const SampleFormat &format) :
  AudioParams(params),
  format_(format)
{
}

const SampleFormat &AudioRenderingParams::format() const
{
  return format_;
}

bool AudioRenderingParams::operator==(const AudioRenderingParams &other)
{
  return (format() == other.format()
          && sample_rate() == other.sample_rate()
          && channel_layout() == other.channel_layout());
}

bool AudioRenderingParams::operator!=(const AudioRenderingParams &other)
{
  return (format() != other.format()
          || sample_rate() != other.sample_rate()
          || channel_layout() != other.channel_layout());
}

int AudioRenderingParams::time_to_bytes(const rational &time) const
{
  Q_ASSERT(is_valid());

  return time_to_samples(time) * channel_count() * bytes_per_sample_per_channel();
}

int AudioRenderingParams::time_to_samples(const rational &time) const
{
  Q_ASSERT(is_valid());

  return qFloor(time.toDouble() * sample_rate());
}

int AudioRenderingParams::samples_to_bytes(const int &samples) const
{
  Q_ASSERT(is_valid());

  return samples * channel_count() * bytes_per_sample_per_channel();
}

int AudioRenderingParams::bytes_to_samples(const int &bytes) const
{
  Q_ASSERT(is_valid());

  return bytes / (channel_count() * bytes_per_sample_per_channel());
}

int AudioRenderingParams::channel_count() const
{
  return av_get_channel_layout_nb_channels(channel_layout());
}

int AudioRenderingParams::bytes_per_sample_per_channel() const
{
  switch (format_) {
  case SAMPLE_FMT_U8:
    return 1;
  case SAMPLE_FMT_S16:
    return 2;
  case SAMPLE_FMT_S32:
  case SAMPLE_FMT_FLT:
    return 4;
  case SAMPLE_FMT_DBL:
  case SAMPLE_FMT_S64:
    return 8;
  case SAMPLE_FMT_INVALID:
  case SAMPLE_FMT_COUNT:
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
  bool valid = (sample_rate() > 0
                && channel_layout() > 0
                && format_ != SAMPLE_FMT_INVALID
                && format_ != SAMPLE_FMT_COUNT);

  if (!valid) {
    qWarning() << "Invalid params found:" << sample_rate() << channel_layout() << format();
  }

  return valid;
}

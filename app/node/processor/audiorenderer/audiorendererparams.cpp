#include "audiorendererparams.h"

AudioRendererParams::AudioRendererParams(const int &sample_rate, const uint64_t &channel_layout, const olive::SampleFormat &format) :
  sample_rate_(sample_rate),
  channel_layout_(channel_layout),
  format_(format)
{
}

const int &AudioRendererParams::sample_rate()
{
  return sample_rate_;
}

const uint64_t &AudioRendererParams::channel_layout()
{
  return channel_layout_;
}

const olive::SampleFormat &AudioRendererParams::format()
{
  return format_;
}

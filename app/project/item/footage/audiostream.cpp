#include "audiostream.h"

AudioStream::AudioStream(Footage* footage,
                         const int& channels,
                         const int& layout,
                         const int& sample_rate) :
  footage_(footage),
  channels_(channels),
  layout_(layout),
  sample_rate_(sample_rate)
{

}

Footage *AudioStream::footage()
{
  return footage_;
}

void AudioStream::set_footage(Footage *f)
{
  footage_ = f;
}

const int &AudioStream::channels()
{
  return channels_;
}

void AudioStream::set_channels(const int &channels)
{
  channels_ = channels;
}

const int &AudioStream::layout()
{
  return layout_;
}

void AudioStream::set_layout(const int &layout)
{
  layout_ = layout;
}

const int &AudioStream::sample_rate()
{
  return sample_rate_;
}

void AudioStream::set_sample_rate(const int &sample_rate)
{
  sample_rate_ = sample_rate;
}

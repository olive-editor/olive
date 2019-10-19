#ifndef AUDIORENDERERPARAMS_H
#define AUDIORENDERERPARAMS_H

#include <QtGlobal>

#include "audio/sampleformat.h"

class AudioRendererParams
{
public:
  AudioRendererParams(const int& sample_rate, const uint64_t& channel_layout, const olive::SampleFormat& format);

  const int& sample_rate();

  const uint64_t& channel_layout();

  const olive::SampleFormat& format();

private:
  int sample_rate_;

  uint64_t channel_layout_;

  olive::SampleFormat format_;

};

#endif // AUDIORENDERERPARAMS_H

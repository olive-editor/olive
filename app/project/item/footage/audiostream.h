#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include "rational.h"

class Footage;

class AudioStream
{
public:
  AudioStream(Footage* footage, const int& channels, const int& layout, const int& sample_rate);

  Footage* footage();
  void set_footage(Footage* f);

  const int& channels();
  void set_channels(const int& channels);

  const int& layout();
  void set_layout(const int& layout);

  const int& sample_rate();
  void set_sample_rate(const int& sample_rate);

private:
  Footage* footage_;

  int channels_;
  int layout_;
  int sample_rate_;
};

#endif // AUDIOSTREAM_H

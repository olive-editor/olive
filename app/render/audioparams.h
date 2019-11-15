#ifndef AUDIOPARAMS_H
#define AUDIOPARAMS_H

#include <QtMath>

#include "audio/sampleformat.h"
#include "common/rational.h"

class AudioParams
{
public:
  AudioParams();
  AudioParams(const int& sample_rate, const uint64_t& channel_layout);

  const int& sample_rate() const;
  const uint64_t& channel_layout() const;

private:
  int sample_rate_;

  uint64_t channel_layout_;

};

class AudioRenderingParams : public AudioParams {
public:
  AudioRenderingParams();
  AudioRenderingParams(const int& sample_rate, const uint64_t& channel_layout, const SampleFormat& format);
  AudioRenderingParams(const AudioParams& params, const SampleFormat& format);

  int time_to_bytes(const rational& time) const;
  int time_to_samples(const rational& time) const;
  int samples_to_bytes(const int& samples) const;
  int channel_count() const;
  int bytes_per_sample_per_channel() const;
  int bits_per_sample() const;
  bool is_valid() const;

  const SampleFormat& format() const;

private:
  SampleFormat format_;
};

#endif // AUDIOPARAMS_H

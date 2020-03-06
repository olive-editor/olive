#include "sumsamples.h"

#include <QDebug>

const int SampleSummer::kSumSampleRate = 200;

QVector<SampleSummer::Sum> SampleSummer::SumSamples(const float *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<float>(samples, nb_samples, nb_channels);
}

QVector<SampleSummer::Sum> SampleSummer::SumSamples(const qfloat16 *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<qfloat16>(samples, nb_samples, nb_channels);
}

QVector<SampleSummer::Sum> SampleSummer::ReSumSamples(const SampleSummer::Sum *samples, int nb_samples, int nb_channels)
{
  QVector<SampleSummer::Sum> summed_samples(nb_channels);

  for (int i=0;i<nb_samples;i++) {
    int channel = i%nb_channels;

    const SampleSummer::Sum& sample = samples[i];

    if (sample.min < summed_samples[channel].min) {
      summed_samples[channel].min = sample.min;
    }

    if (sample.max > summed_samples[channel].max) {
      summed_samples[channel].max = sample.max;
    }
  }

  return summed_samples;
}

SampleSummer::Sum::Sum()
{
  min = 0;
  max = 0;
}

template<typename T>
QVector<SampleSummer::Sum> SampleSummer::SumSamplesInternal(const T *samples, int nb_samples, int nb_channels)
{
  QVector<SampleSummer::Sum> summed_samples(nb_channels);

  for (int i=0;i<nb_samples;i++) {
    int channel = i%nb_channels;

    float sample = samples[i];

    if (sample < summed_samples[channel].min) {
      summed_samples[channel].min = sample;
    }

    if (sample > summed_samples[channel].max) {
      summed_samples[channel].max = sample;
    }
  }

  return summed_samples;
}

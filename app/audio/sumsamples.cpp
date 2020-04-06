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

#include "sumsamples.h"

#include <QDebug>

OLIVE_NAMESPACE_ENTER

const int SampleSummer::kSumSampleRate = 200;

QVector<SampleSummer::Sum> SampleSummer::SumSamples(const float *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<float>(samples, nb_samples, nb_channels);
}

QVector<SampleSummer::Sum> SampleSummer::SumSamples(const qfloat16 *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<qfloat16>(samples, nb_samples, nb_channels);
}

QVector<SampleSummer::Sum> SampleSummer::SumSamples(SampleBufferPtr samples, int start_index, int length)
{
  QVector<SampleSummer::Sum> summed_samples(samples->audio_params().channel_count());

  int end_index = start_index + length;

  for (int i=start_index;i<end_index;i++) {
    for (int channel=0;channel<samples->audio_params().channel_count();channel++) {
      ClampMinMax<float>(summed_samples[channel], samples->data()[channel][i]);
    }
  }

  return summed_samples;
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
    ClampMinMax<T>(summed_samples[i%nb_channels], samples[i]);
  }

  return summed_samples;
}

template<typename T>
void SampleSummer::ClampMinMax(SampleSummer::Sum &sum, T value)
{
  if (value < sum.min) {
    sum.min = value;
  }

  if (value > sum.max) {
    sum.max = value;
  }
}

SampleSummer::Info::Info()
{
  channels = 0;
}

OLIVE_NAMESPACE_EXIT

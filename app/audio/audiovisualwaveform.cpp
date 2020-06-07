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

#include "audiovisualwaveform.h"

#include <QDebug>

#include "config/config.h"

OLIVE_NAMESPACE_ENTER

const int AudioVisualWaveform::kSumSampleRate = 200;

void AudioVisualWaveform::AddSamples(SampleBufferPtr samples, int sample_rate)
{
  if (!channels_) {
    qWarning() << "Failed to write samples - channel count is zero";
  }

  overwrite_samples_internal(samples, sample_rate, 0);
}

void AudioVisualWaveform::AddSum(const float *samples, int nb_samples, int nb_channels)
{
  data_.append(SumSamples(samples, nb_samples, nb_channels));
}

void AudioVisualWaveform::OverwriteSamples(SampleBufferPtr samples, int sample_rate, const rational &start)
{
  if (!channels_) {
    qWarning() << "Failed to write samples - channel count is zero";
  }

  overwrite_samples_internal(samples, sample_rate, time_to_samples(start));
}

void AudioVisualWaveform::OverwriteSums(const AudioVisualWaveform &sums, const rational &dest, const rational& offset, const rational& length)
{
  if (sums.data_.isEmpty()) {
    return;
  }

  int start_index = time_to_samples(dest);
  int sample_start = time_to_samples(offset);

  int copy_len = sums.data_.size() - sample_start;
  if (!length.isNull()) {
    copy_len = qMin(copy_len, time_to_samples(length));
  }

  int end_index = start_index + copy_len;

  if (data_.size() < end_index) {
    data_.resize(end_index);
  }

  memcpy(reinterpret_cast<char*>(data_.data()) + start_index * sizeof(SamplePerChannel),
         reinterpret_cast<const char*>(sums.data_.constData()) + time_to_samples(offset) * sizeof(SamplePerChannel),
         copy_len * sizeof(SamplePerChannel));
}

AudioVisualWaveform AudioVisualWaveform::Mid(const rational &time) const
{
  int sample_index = time_to_samples(time);

  // Create a copy of this waveform chop the early section off
  AudioVisualWaveform copy = *this;
  copy.data_ = data_.mid(sample_index);

  return copy;
}

void AudioVisualWaveform::Append(const AudioVisualWaveform &waveform)
{
  data_.append(waveform.data_);
}

void AudioVisualWaveform::TrimIn(const rational &time)
{
  data_ = data_.mid(time_to_samples(time));
}

void AudioVisualWaveform::TrimOut(const rational &time)
{
  data_.resize(data_.size() - time_to_samples(time));
}

void AudioVisualWaveform::PrependSilence(const rational &time)
{
  int added_samples = time_to_samples(time);

  // Resize buffer for extra space
  data_.resize(data_.size() + added_samples);

  // Shift all data forward
  for (int i=data_.size()-1; i>=added_samples; i--) {
    data_[i] = data_[i - added_samples];
  }

  // Fill remainder with silence
  memset(reinterpret_cast<char*>(data_.data()), 0, added_samples * sizeof(SamplePerChannel));
}

void AudioVisualWaveform::AppendSilence(const rational &time)
{
  int added_samples = time_to_samples(time);

  // Resize buffer for extra space
  int old_size = data_.size();
  data_.resize(old_size + added_samples);

  // Fill remainder with silence
  memset(reinterpret_cast<char*>(&data_[old_size]), 0, (data_.size() - old_size) * sizeof(SamplePerChannel));
}

void AudioVisualWaveform::Shift(const rational &from, const rational &to)
{
  int from_index = time_to_samples(from);
  int to_index = time_to_samples(to);

  if (from_index == to_index) {
    return;
  }

  if (from_index > to_index) {
    // Shifting backwards <-
    int copy_sz = data_.size() - from_index;

    for (int i=0; i<copy_sz; i++) {
      data_.replace(to_index + i, data_.at(from_index + i));
    }

    data_.resize(data_.size() - (from_index - to_index));
  } else {
    // Shifting forwards ->
    int old_sz = data_.size();

    int distance = (to_index - from_index);

    data_.resize(data_.size() + distance);

    int copy_sz = old_sz - from_index;

    for (int i=0; i<copy_sz; i++) {
      data_.replace(data_.size() - i - 1, data_.at(old_sz - i - 1));
    }

    memset(reinterpret_cast<char*>(&data_[from_index]), 0, distance * sizeof(SamplePerChannel));
  }
}

QVector<AudioVisualWaveform::SamplePerChannel> AudioVisualWaveform::SumSamples(const float *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<float>(samples, nb_samples, nb_channels);
}

QVector<AudioVisualWaveform::SamplePerChannel> AudioVisualWaveform::SumSamples(const qfloat16 *samples, int nb_samples, int nb_channels)
{
  return SumSamplesInternal<qfloat16>(samples, nb_samples, nb_channels);
}

QVector<AudioVisualWaveform::SamplePerChannel> AudioVisualWaveform::SumSamples(SampleBufferPtr samples, int start_index, int length)
{
  QVector<AudioVisualWaveform::SamplePerChannel> summed_samples(samples->audio_params().channel_count());

  int end_index = start_index + length;

  for (int i=start_index;i<end_index;i++) {
    for (int channel=0;channel<samples->audio_params().channel_count();channel++) {
      ClampMinMax<float>(summed_samples[channel], samples->data()[channel][i]);
    }
  }

  return summed_samples;
}

QVector<AudioVisualWaveform::SamplePerChannel> AudioVisualWaveform::ReSumSamples(const SamplePerChannel* samples,
                                                                                 int nb_samples,
                                                                                 int nb_channels)
{
  QVector<AudioVisualWaveform::SamplePerChannel> summed_samples(nb_channels);

  for (int i=0;i<nb_samples;i++) {
    int channel = i%nb_channels;

    const AudioVisualWaveform::SamplePerChannel& sample = samples[i];

    if (sample.min < summed_samples[channel].min) {
      summed_samples[channel].min = sample.min;
    }

    if (sample.max > summed_samples[channel].max) {
      summed_samples[channel].max = sample.max;
    }
  }

  return summed_samples;
}

void AudioVisualWaveform::DrawSample(QPainter *painter, const QVector<SamplePerChannel>& sample, int x, int y, int height)
{
  int channel_height = height / sample.size();
  int channel_half_height = channel_height / 2;

  for (int i=0;i<sample.size();i++) {
    if (Config::Current()[QStringLiteral("RectifiedWaveforms")].toBool()) {
      int channel_bottom = y + channel_height * (i + 1);

      int diff = qRound((sample.at(i).max - sample.at(i).min) * channel_half_height);

      painter->drawLine(x,
                        channel_bottom - diff,
                        x,
                        channel_bottom);
    } else {
      int channel_mid = y + channel_height * i + channel_half_height;

      painter->drawLine(x,
                        channel_mid + qRound(sample.at(i).min * static_cast<float>(channel_half_height)),
                        x,
                        channel_mid + qRound(sample.at(i).max * static_cast<float>(channel_half_height)));
    }
  }
}

void AudioVisualWaveform::DrawWaveform(QPainter *painter, const QRect& rect, const double& scale, const AudioVisualWaveform &samples, const rational& start_time)
{
  int start_sample_index = samples.time_to_samples(start_time);
  int next_sample_index = start_sample_index;
  int sample_index;

  QVector<SamplePerChannel> summary;
  int summary_index = -1;

  const QRect& viewport = painter->viewport();
  QPoint top_left = painter->transform().map(viewport.topLeft());

  int start = qMax(rect.x(), -top_left.x());
  int end = qMin(rect.right(), -top_left.x() + viewport.width());

  for (int i=start;i<end;i++) {
    sample_index = next_sample_index;

    if (sample_index == samples.nb_samples()) {
      break;
    }

    next_sample_index = qMin(samples.nb_samples(),
                             start_sample_index + qFloor(static_cast<double>(kSumSampleRate) * static_cast<double>(i - rect.x() + 1) / scale) * samples.channel_count());

    if (summary_index != sample_index) {
      summary = AudioVisualWaveform::ReSumSamples(&samples.data_.at(sample_index),
                                                  qMax(samples.channel_count(), next_sample_index - sample_index),
                                                  samples.channel_count());
      summary_index = sample_index;
    }

    DrawSample(painter, summary, i, rect.y(), rect.height());
  }
}

void AudioVisualWaveform::overwrite_samples_internal(SampleBufferPtr samples, int sample_rate, int start_index)
{
  int samples_length = time_to_samples(static_cast<double>(samples->sample_count()) / static_cast<double>(sample_rate));

  int end_index = start_index + samples_length;
  if (data_.size() < end_index) {
    data_.resize(end_index);
  }

  int chunk_size = sample_rate / kSumSampleRate;

  for (int i=0; i<samples_length; i++) {

    int src_index = (i * chunk_size) / channels_;

    QVector<SamplePerChannel> summary = SumSamples(samples,
                                                   src_index,
                                                   qMin(chunk_size, samples->sample_count() - src_index));

    memcpy(&data_.data()[i + start_index],
        summary.constData(),
        summary.size() * sizeof(SamplePerChannel));
  }
}

int AudioVisualWaveform::time_to_samples(const rational &time) const
{
  return time_to_samples(time.toDouble());
}

int AudioVisualWaveform::time_to_samples(const double &time) const
{
  return qFloor(time * kSumSampleRate) * channels_;
}

template<typename T>
QVector<AudioVisualWaveform::SamplePerChannel> AudioVisualWaveform::SumSamplesInternal(const T *samples, int nb_samples, int nb_channels)
{
  QVector<AudioVisualWaveform::SamplePerChannel> summed_samples(nb_channels);

  for (int i=0;i<nb_samples;i++) {
    ClampMinMax<T>(summed_samples[i%nb_channels], samples[i]);
  }

  return summed_samples;
}

template<typename T>
void AudioVisualWaveform::ClampMinMax(AudioVisualWaveform::SamplePerChannel &sum, T value)
{
  if (value < sum.min) {
    sum.min = value;
  }

  if (value > sum.max) {
    sum.max = value;
  }
}

OLIVE_NAMESPACE_EXIT

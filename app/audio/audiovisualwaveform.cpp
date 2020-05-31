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

void AudioVisualWaveform::AddSum(const float *samples, int nb_samples, int nb_channels)
{
  data_.append(SumSamples(samples, nb_samples, nb_channels));
}

/*
void AudioVisualWaveform::AddSamples(SampleBufferPtr samples)
{
  if (!params_.is_valid()) {

  }

  int chunk_size = (audio_params_.sample_rate() / waveform_params.sample_rate());

  qint64 start_offset = sizeof(SampleSummer::Info) + waveform_params.time_to_bytes(range_for_block.in() - b->in());
  qint64 length_offset = waveform_params.time_to_bytes(range_for_block.length());
  qint64 end_offset = start_offset + length_offset;

  if (wave_file.size() < end_offset) {
    wave_file.resize(end_offset);
  }

  wave_file.seek(start_offset);

  for (int i=0;i<samples->sample_count();i+=chunk_size) {
    QVector<Sample> summary = SumSamples(samples,
                                         i,
                                         qMin(chunk_size, samples->sample_count_per_channel() - i));

    wave_file.write(reinterpret_cast<const char*>(summary.constData()),
                    summary.size() * sizeof(SampleSummer::Sum));
  }
}
*/

void AudioVisualWaveform::OverwriteSamples(SampleBufferPtr samples, int sample_rate, const rational &start)
{
  if (!channels_) {
    qWarning() << "Failed to write samples - channel count is zero";
  }

  int start_index = channels_ * qFloor(kSumSampleRate * start.toDouble());
  int samples_length = channels_ * qFloor(kSumSampleRate * (static_cast<double>(samples->sample_count()) / static_cast<double>(sample_rate)));

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

AudioVisualWaveform AudioVisualWaveform::Cut(const rational &time)
{
  int sample_index = time_to_samples(time);

  // Create a copy of this waveform chop the early section off
  AudioVisualWaveform copy = *this;
  copy.data_ = data_.mid(sample_index);

  // Chop the latter section off too
  data_.resize(sample_index);

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
  for (int i=0;i<added_samples;i++) {
    data_[i].max = 0;
    data_[i].min = 0;
  }
}

void AudioVisualWaveform::AppendSilence(const rational &time)
{
  int added_samples = time_to_samples(time);

  // Resize buffer for extra space
  int old_size = data_.size();
  data_.resize(old_size + added_samples);

  // Fill remainder with silence
  for (int i=old_size;i<data_.size();i++) {
    data_[i].max = 0;
    data_[i].min = 0;
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

void AudioVisualWaveform::DrawWaveform(QPainter *painter, const QRect& rect, const double& scale, const AudioVisualWaveform &samples)
{
  int sample_index, next_sample_index = 0;

  QVector<SamplePerChannel> summary;
  int summary_index = -1;

  const QRect& viewport = painter->viewport();
  QPoint top_left = painter->transform().map(viewport.topLeft());

  int start = qMax(rect.x(), -top_left.x());
  int end = qMin(rect.right(), -top_left.x() + viewport.width());

  QVector<QLine> lines;

  for (int i=start;i<end;i++) {
    sample_index = next_sample_index;

    if (sample_index == samples.nb_samples()) {
      break;
    }

    next_sample_index = qMin(samples.nb_samples(),
                             qFloor(static_cast<double>(kSumSampleRate) * static_cast<double>(i - rect.x() + 1) / scale) * samples.channel_count());

    if (summary_index != sample_index) {
      summary = AudioVisualWaveform::ReSumSamples(&samples.data_.at(sample_index),
                                                  qMax(samples.channel_count(), next_sample_index - sample_index),
                                                  samples.channel_count());
      summary_index = sample_index;
    }

    DrawSample(painter, summary, i, rect.y(), rect.height());
  }

  painter->drawLines(lines);
}

int AudioVisualWaveform::time_to_samples(const rational &time) const
{
  return qFloor(time.toDouble() * kSumSampleRate) * channels_;
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

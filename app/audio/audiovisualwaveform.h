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

#ifndef SUMSAMPLES_H
#define SUMSAMPLES_H

#include <QFloat16>
#include <QPainter>
#include <QVector>

#include "codec/samplebuffer.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A buffer of data used to store a visual representation of audio
 *
 * This differs from a SampleBuffer as the data in an AudioVisualWaveform has been reduced
 * significantly and optimized for visual display.
 */
class AudioVisualWaveform {
public:
  AudioVisualWaveform() = default;

  struct SamplePerChannel {
    qfloat16 min = 0;
    qfloat16 max = 0;
  };

  using Sample = QVector<SamplePerChannel>;

  int channel_count() const
  {
    return channels_;
  }

  void set_channel_count(int channels)
  {
    channels_ = channels;
  }

  int nb_samples() const
  {
    return data_.size();
  }

  const SamplePerChannel* const_data() const
  {
    return data_.constData();
  }

  void AddSum(const float* samples, int nb_samples, int nb_channels);

  void OverwriteSamples(SampleBufferPtr samples, int sample_rate, const rational& start);
  void OverwriteSums(const AudioVisualWaveform& sums, const rational& start);

  AudioVisualWaveform Mid(const rational& time) const;
  void Append(const AudioVisualWaveform& waveform);
  void TrimIn(const rational& time);
  void TrimOut(const rational& time);
  void PrependSilence(const rational& time);
  void AppendSilence(const rational& time);

  // FIXME: Move to dynamic
  static const int kSumSampleRate;

  static QVector<SamplePerChannel> SumSamples(const float* samples, int nb_samples, int nb_channels);
  static QVector<SamplePerChannel> SumSamples(const qfloat16* samples, int nb_samples, int nb_channels);
  static QVector<SamplePerChannel> SumSamples(SampleBufferPtr samples, int start_index, int length);

  static QVector<SamplePerChannel> ReSumSamples(const SamplePerChannel *samples, int nb_samples, int nb_channels);

  static void DrawSample(QPainter* painter, const QVector<SamplePerChannel> &sample, int x, int y, int height);

  static void DrawWaveform(QPainter* painter, const QRect &rect, const double &scale, const AudioVisualWaveform& samples);

private:
  template <typename T>
  static QVector<SamplePerChannel> SumSamplesInternal(const T* samples, int nb_samples, int nb_channels);

  template <typename T>
  static void ClampMinMax(SamplePerChannel &sum, T value);

  int time_to_samples(const rational& time) const;
  int time_to_samples(const double& time) const;

  int channels_ = 0;

  QVector<SamplePerChannel> data_;

};

OLIVE_NAMESPACE_EXIT

#endif // SUMSAMPLES_H

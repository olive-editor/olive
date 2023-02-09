/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include <olive/core/core.h>
#include <QPainter>
#include <QVector>

namespace olive {

using namespace core;

/**
 * @brief A buffer of data used to store a visual representation of audio
 *
 * This differs from a SampleBuffer as the data in an AudioVisualWaveform has been reduced
 * significantly and optimized for visual display.
 */
class AudioVisualWaveform {
public:
  AudioVisualWaveform();

  struct SamplePerChannel {
    float min;
    float max;
  };

  using Sample = std::vector<SamplePerChannel>;

  int channel_count() const
  {
    return channels_;
  }

  void set_channel_count(int channels)
  {
    channels_ = channels;
  }

  const rational& length() const
  {
    return length_;
  }

  /**
   * @brief Writes samples into the visual waveform buffer
   *
   * Starting at `start`, writes samples over anything in the buffer, expanding it if necessary.
   */
  void OverwriteSamples(const SampleBuffer &samples, int sample_rate, const rational& start = 0);

  /**
   * @brief Replaces sums at a certain range in this visual waveform
   *
   * @param sums
   *
   * The sums to write over our current ones with.
   *
   * @param dest
   *
   * Where in this visual waveform these sums should START being written to.
   *
   * @param offset
   *
   * Where in the `sums` parameter this should start reading from. Defaults to 0.
   *
   * @param length
   *
   * Maximum length of `sums` to overwrite with.
   */
  void OverwriteSums(const AudioVisualWaveform& sums, const rational& dest, const rational& offset = 0, const rational &length = 0);

  void OverwriteSilence(const rational &start, const rational &length);

  void TrimIn(rational length);

  AudioVisualWaveform Mid(const rational &offset) const;
  AudioVisualWaveform Mid(const rational &offset, const rational &length) const;

  void Resize(const rational &length);

  void TrimRange(const rational &in, const rational &length);

  Sample GetSummaryFromTime(const rational& start, const rational& length) const;

  static Sample SumSamples(const SampleBuffer &samples, size_t start_index, size_t length);

  static Sample ReSumSamples(const SamplePerChannel *samples, size_t nb_samples, int nb_channels);

  static void DrawSample(QPainter* painter, const Sample &sample, int x, int y, int height, bool rectified);

  static void DrawWaveform(QPainter* painter, const QRect &rect, const double &scale, const AudioVisualWaveform& samples, const rational &start_time);

  // Must be a power of 2
  static const rational kMinimumSampleRate;
  static const rational kMaximumSampleRate;

private:
  void OverwriteSamplesFromBuffer(const SampleBuffer &samples, int sample_rate, const rational& start, double target_rate, Sample &data, size_t &start_index, size_t &samples_length);

  void OverwriteSamplesFromMipmap(const Sample& input, double input_sample_rate, size_t &input_start, size_t &input_length, const rational& start, double output_rate, Sample &output_data);

  size_t time_to_samples(const rational& time, double sample_rate) const;
  size_t time_to_samples(const double& time, double sample_rate) const;

  std::map<rational, Sample>::const_iterator GetMipmapForScale(double scale) const;

  void ValidateVirtualStart(const rational &new_start);

  rational virtual_start_;

  int channels_;

  std::map<rational, Sample> mipmapped_data_;

  rational length_;

};

}

Q_DECLARE_METATYPE(olive::AudioVisualWaveform)

#endif // SUMSAMPLES_H

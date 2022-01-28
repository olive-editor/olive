/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QtGlobal>

#include "config/config.h"
#include "common/cpuoptimize.h"
#include "common/functiontimer.h"

namespace olive {

const rational AudioVisualWaveform::kMinimumSampleRate = rational(1, 8);
const rational AudioVisualWaveform::kMaximumSampleRate = 1024;

AudioVisualWaveform::AudioVisualWaveform() :
  channels_(0)
{
  for (rational i=kMinimumSampleRate; i<=kMaximumSampleRate; i*=2) {
    mipmapped_data_.insert({i, Sample()});
  }
}

void AudioVisualWaveform::OverwriteSamplesFromBuffer(SampleBufferPtr samples, int sample_rate, const rational &start, double target_rate, Sample& data, int &start_index, int &samples_length)
{
  start_index = time_to_samples(start, target_rate);
  samples_length = time_to_samples(static_cast<double>(samples->sample_count()) / static_cast<double>(sample_rate), target_rate);

  int end_index = start_index + samples_length;
  if (data.size() < end_index) {
    data.resize(end_index);
  }

  double chunk_size = double(sample_rate) / double(target_rate);

  for (int i=0; i<samples_length; i+=channels_) {
    int src_start = qRound((double(i) * chunk_size)) / channels_;
    int src_end = qMin(qRound((double(i + channels_) * chunk_size)) / channels_, samples->sample_count());

    Sample summary = SumSamples(samples,
                                src_start,
                                src_end - src_start);

    memcpy(&data.data()[i + start_index],
        summary.constData(),
        summary.size() * sizeof(SamplePerChannel));
  }
}

void AudioVisualWaveform::OverwriteSamplesFromMipmap(const AudioVisualWaveform::Sample &input, double input_sample_rate, int &input_start, int &input_length, const rational &start, double output_rate, AudioVisualWaveform::Sample &output_data)
{
  int start_index = time_to_samples(start, output_rate);
  int samples_length = time_to_samples(static_cast<double>(input_length / channels_) / input_sample_rate, output_rate);

  int end_index = start_index + samples_length;
  if (output_data.size() < end_index) {
    output_data.resize(end_index);
  }

  // We guarantee mipmaps are powers of two so integer division should be perfectly accurate here
  int chunk_size = input_sample_rate / output_rate;

  for (int i=0; i<samples_length; i+=channels_) {
    Sample summary = ReSumSamples(&input.constData()[input_start + (i*chunk_size)], chunk_size * channels_, channels_);

    memcpy(&output_data.data()[i + start_index],
        summary.constData(),
        summary.size() * sizeof(SamplePerChannel));
  }

  input_start = start_index;
  input_length = samples_length;
}

void AudioVisualWaveform::OverwriteSamples(SampleBufferPtr samples, int sample_rate, const rational &start)
{
  if (!channels_) {
    qWarning() << "Failed to write samples - channel count is zero";
    return;
  }

  // Old less optimized code. Keeping this around as a reference, but the below code is at least
  // 10x faster so this shouldn't be used in production.
  //
  // int input_start, input_length;
  // for (auto it=mipmapped_data_.begin(); it!=mipmapped_data_.end(); it++) {
  //   OverwriteSamplesFromBuffer(samples, sample_rate, start, it->first.toDouble(), it->second, input_start, input_length);
  // }

  // Process the largest mipmap directly for the samples
  auto current_mipmap = mipmapped_data_.rbegin();
  int input_start, input_length;
  OverwriteSamplesFromBuffer(samples, sample_rate, start, current_mipmap->first.toDouble(), current_mipmap->second, input_start, input_length);

  while (true) {
    // For each smaller mipmap, we just process from the mipmap before it, making each one
    // exponentially faster to create
    auto previous_mipmap = current_mipmap;
    current_mipmap++;
    if (current_mipmap == mipmapped_data_.rend()) {
      break;
    }

    OverwriteSamplesFromMipmap(previous_mipmap->second, previous_mipmap->first.toDouble(),
                               input_start, input_length, start, current_mipmap->first.toDouble(),
                               current_mipmap->second);
  }

  rational sample_length(samples->sample_count(), sample_rate);
  length_ = qMax(length_, start + sample_length);
}

void AudioVisualWaveform::OverwriteSums(const AudioVisualWaveform &sums, const rational &dest, const rational& offset, const rational& length)
{
  for (auto it=mipmapped_data_.begin(); it!=mipmapped_data_.end(); it++) {
    rational rate = it->first;

    Sample& our_arr = it->second;
    const Sample& their_arr = sums.mipmapped_data_.at(rate);

    double rate_dbl = rate.toDouble();

    // Get our destination sample
    int our_start_index = time_to_samples(dest, rate_dbl);

    // Get our source sample
    int their_start_index = time_to_samples(offset, rate_dbl);
    if (their_start_index >= their_arr.size()) {
      continue;
    }

    // Determine how much we're copying
    int copy_len = their_arr.size() - their_start_index;
    if (!length.isNull()) {
      copy_len = qMin(copy_len, time_to_samples(length, rate_dbl));
    }

    // Determine end index of our array
    int end_index = our_start_index + copy_len;
    if (our_arr.size() < end_index) {
      our_arr.resize(end_index);
    }

    memcpy(reinterpret_cast<char*>(our_arr.data()) + our_start_index * sizeof(SamplePerChannel),
           reinterpret_cast<const char*>(their_arr.constData()) + their_start_index * sizeof(SamplePerChannel),
           copy_len * sizeof(SamplePerChannel));
  }

  length_ = qMax(length_, dest + ((length.isNull()) ? sums.length() - offset : length));
}

void AudioVisualWaveform::OverwriteSilence(const rational &start, const rational &length)
{
  for (auto it=mipmapped_data_.begin(); it!=mipmapped_data_.end(); it++) {
    rational rate = it->first;

    Sample& our_arr = it->second;

    double rate_dbl = rate.toDouble();

    // Get our destination sample
    int our_start_index = time_to_samples(start, rate_dbl);
    int our_length_index = time_to_samples(length, rate_dbl);
    int our_end_index = our_start_index + our_length_index;

    if (our_arr.size() < our_end_index) {
      our_arr.resize(our_end_index);
    }

    memset(reinterpret_cast<char*>(our_arr.data()) + our_start_index * sizeof(SamplePerChannel), 0, our_length_index * sizeof(SamplePerChannel));
  }
}

void AudioVisualWaveform::Shift(const rational &from, const rational &to)
{
  for (auto it=mipmapped_data_.begin(); it!=mipmapped_data_.end(); it++) {
    rational rate = it->first;
    double rate_dbl = rate.toDouble();
    Sample& data = it->second;

    int from_index = time_to_samples(from, rate_dbl);
    int to_index = time_to_samples(to, rate_dbl);

    if (from_index == to_index) {
      continue;
    }

    if (from_index >= data.size()) {
      continue;
    }

    if (from_index > to_index) {
      // Shifting backwards <-
      data.remove(to_index, from_index - to_index);
    } else {
      // Shifting forwards ->
      data.insert(from_index, to_index - from_index, {0, 0});
    }
  }

  length_ = qMax(rational(0), length_ + (to-from));
}

void AudioVisualWaveform::TrimIn(const rational &length)
{
  for (auto it=mipmapped_data_.begin(); it!=mipmapped_data_.end(); it++) {
    rational rate = it->first;
    double rate_dbl = rate.toDouble();
    Sample& data = it->second;

    int chop_length = time_to_samples(length, rate_dbl);

    if (chop_length == 0) {
      continue;
    }

    if (chop_length > 0) {
      data = data.mid(chop_length);
    } else {
      data.insert(0, -chop_length, SamplePerChannel());
    }
  }

  length_ = qMax(rational(0), length_ - length);
}

AudioVisualWaveform AudioVisualWaveform::Mid(const rational &offset) const
{
  AudioVisualWaveform mid  = *this;

  mid.TrimIn(offset);

  return mid;
}

AudioVisualWaveform::Sample AudioVisualWaveform::GetSummaryFromTime(const rational &start, const rational &length) const
{
  // Find mipmap that requires
  auto using_mipmap = GetMipmapForScale(length.flipped().toDouble());

  double rate_dbl = using_mipmap->first.toDouble();

  int start_sample = time_to_samples(start, rate_dbl);
  int sample_length = time_to_samples(length, rate_dbl);

  const QVector<AudioVisualWaveform::SamplePerChannel> &mipmap_data = using_mipmap->second;

  // Determine if the array actually has this sample
  sample_length = qMin(sample_length, mipmap_data.size() - start_sample);

  // Based on the above `min`, if sample length <= 0, that means start_sample >= the size of the
  // array and nothing can be returned.
  if (sample_length > 0) {
    return ReSumSamples(&mipmap_data.constData()[start_sample], sample_length, channels_);
  }

  // Return null samples
  return AudioVisualWaveform::Sample(channel_count(), {0, 0});
}

void ExpandMinMaxChannel(float *a, int start, int length, float &min_val, float &max_val)
{
#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
  // SSE optimized

  // load the first 4 elements of 'a' into min and max (they are 4 * 32 = 128 bits)
  __m128 max = _mm_loadu_ps(a + start);
  __m128 min = _mm_loadu_ps(a + start);

  // loop over 'a' and compare current elements with min and max 4 by 4.
  // we need to make sure we don't read out of boundaries should 'a' lenght be not mod. 4
  for(int i = 4; i < length-4; i+=4) {
    __m128 cur = _mm_loadu_ps(a + start + i);
    max = _mm_max_ps(max, cur);
    min = _mm_min_ps(min, cur);
  }
  // so we read the last 4 (or less) elements in a safe manner.
  __m128 cur = _mm_loadu_ps(a + length - 4);
  max = _mm_max_ps(max, cur);
  min = _mm_min_ps(min, cur);
  // this potentially overlaps up to the last 3 elements but it's not an issue.

  // min and max will contain 4 min and max. To get the absolute min and max
  // we need to compare the 4 values over themselves by shuffling each time.
  for (int i = 0; i < 3; i++) {
    max = _mm_max_ps(max, _mm_shuffle_ps(max, max, 0x93));
    min = _mm_min_ps(min, _mm_shuffle_ps(min, min, 0x93));
  }
  // now min and max contain 4 identical items each representing min and max value respectively.

  // and we store the first one into a float variable.
  _mm_store_ss(&max_val, max);
  _mm_store_ss(&min_val, min);
  // I bet you don't find annotated low level code very often.
#else
  // Standard unoptimized function
  int end = start + length;
  for (int i=start; i<end; i++) {
    min_val = std::min(min_val, a[i]);
    max_val = std::max(max_val, a[i]);
  }
#endif
}

AudioVisualWaveform::Sample AudioVisualWaveform::SumSamples(SampleBufferPtr samples, int start_index, int length)
{
  int channels = samples->audio_params().channel_count();
  AudioVisualWaveform::Sample summed_samples(channels);

  for (int channel=0; channel<samples->audio_params().channel_count(); channel++) {
    ExpandMinMaxChannel(samples->data(channel), start_index, length, summed_samples[channel].min, summed_samples[channel].max);
  }

  // for reference: this approximation is n x faster (and less accurate) for a n-tracks clip
  // for (int i=start_index; i<end_index; i++) {
  //   ExpandMinMax(summed_samples[i%channels], samples->data(i%channels)[i]);
  // }

  return summed_samples;
}

AudioVisualWaveform::Sample AudioVisualWaveform::ReSumSamples(const SamplePerChannel* samples,
                                                                                 int nb_samples,
                                                                                 int nb_channels)
{
  AudioVisualWaveform::Sample summed_samples(nb_channels);

  for (int i=0;i<nb_samples;i+=nb_channels) {
    for (int j=0;j<nb_channels;j++) {
      const AudioVisualWaveform::SamplePerChannel& sample = samples[i + j];

      if (sample.min < summed_samples[j].min) {
        summed_samples[j].min = sample.min;
      }

      if (sample.max > summed_samples[j].max) {
        summed_samples[j].max = sample.max;
      }
    }
  }

  return summed_samples;
}

void AudioVisualWaveform::DrawSample(QPainter *painter, const Sample& sample, int x, int y, int height, bool rectified)
{
  if (sample.isEmpty()) {
    return;
  }

  int channel_height = height / sample.size();
  int channel_half_height = channel_height / 2;

  for (int i=0;i<sample.size();i++) {
    float max = qMin(sample.at(i).max, 1.0f);
    float min = qMax(sample.at(i).min, -1.0f);

    if (rectified) {
      int channel_bottom = y + channel_height * (i + 1);

      int diff = qRound((max - min) * channel_half_height);

      painter->drawLine(x,
                        channel_bottom - diff,
                        x,
                        channel_bottom);
    } else {
      int channel_mid = y + channel_height * i + channel_half_height;

      // We subtract the sample so that positive Y values go up on the screen rather than down,
      // which is how waveforms are usually rendered
      painter->drawLine(x,
                        channel_mid - qRound(min * static_cast<float>(channel_half_height)),
                        x,
                        channel_mid - qRound(max * static_cast<float>(channel_half_height)));
    }
  }
}

void AudioVisualWaveform::DrawWaveform(QPainter *painter, const QRect& rect, const double& scale, const AudioVisualWaveform &samples, const rational& start_time)
{
  if (samples.mipmapped_data_.empty()) {
    return;
  }

  auto using_mipmap = samples.GetMipmapForScale(scale);

  rational rate = using_mipmap->first;
  double rate_dbl = rate.toDouble();
  const Sample& arr = using_mipmap->second;

  int start_sample_index = samples.time_to_samples(start_time, rate_dbl);

  if (start_sample_index >= arr.size()) {
    return;
  }

  int next_sample_index = start_sample_index;
  int sample_index;

  Sample summary;
  int summary_index = -1;

  const QRect& viewport = painter->viewport();
  QPoint top_left = painter->transform().map(viewport.topLeft());

  int start = qMax(rect.x(), -top_left.x());
  int end = qMin(rect.right(), -top_left.x() + viewport.width());

  bool rectified = Config::Current()[QStringLiteral("RectifiedWaveforms")].toBool();

  for (int i=start;i<end;i++) {
    sample_index = next_sample_index;

    if (sample_index == arr.size()) {
      break;
    }

    next_sample_index = qMin(arr.size(),
                             start_sample_index + qFloor(rate_dbl * static_cast<double>(i - rect.x() + 1) / scale) * samples.channel_count());

    if (summary_index != sample_index) {
      summary = AudioVisualWaveform::ReSumSamples(&arr.at(sample_index),
                                                  qMax(samples.channel_count(), next_sample_index - sample_index),
                                                  samples.channel_count());
      summary_index = sample_index;
    }

    DrawSample(painter, summary, i, rect.y(), rect.height(), rectified);
  }
}

int AudioVisualWaveform::time_to_samples(const rational &time, double sample_rate) const
{
  return time_to_samples(time.toDouble(), sample_rate);
}

int AudioVisualWaveform::time_to_samples(const double &time, double sample_rate) const
{
  return qFloor(time * sample_rate) * channels_;
}

std::map<rational, AudioVisualWaveform::Sample>::const_iterator AudioVisualWaveform::GetMipmapForScale(double scale) const
{
  // Find largest mipmap for this scale (or the largest if we don't find one sufficient)
  for (auto it=mipmapped_data_.cbegin(); it!=mipmapped_data_.cend(); it++) {
    if (it->first.toDouble() >= scale) {
      return it;
    }
  }

  // We don't have a mipmap large enough for this scale, so just return the largest we have
  return std::prev(mipmapped_data_.cend());
}

}

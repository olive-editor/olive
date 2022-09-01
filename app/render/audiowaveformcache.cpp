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

#include "audiowaveformcache.h"

namespace olive {

AudioWaveformCache::AudioWaveformCache(QObject *parent) :
  PlaybackCache{parent}
{
}

void AudioWaveformCache::WriteWaveform(const TimeRange &range, const TimeRangeList &valid_ranges, const AudioVisualWaveform *waveform)
{
  // Write each valid range to the segments
  foreach (const TimeRange& r, valid_ranges) {
    // Write visual
    TimeRangeList::util_remove(&waveforms_, r);

    if (waveform) {
      TimeRangeWithWaveform wv = r;
      rational local_start = r.in() - range.in();
      wv.waveform = waveform->Mid(local_start, r.length());
      waveforms_.append(wv);
    }

    Validate(r);
  }
}

void AudioWaveformCache::Draw(QPainter *painter, const QRect &rect, const double &scale, const rational &start_time) const
{
  rational end = start_time + rational::fromDouble(rect.width() / scale);
  TimeRange draw_range(start_time, end);

  foreach (const TimeRangeWithWaveform &wv, waveforms_) {
    if (wv.OverlapsWith(draw_range)) {
      rational substart = std::max(wv.in(), draw_range.in());
      rational subend = std::min(wv.out(), draw_range.out());

      QRect subrect = rect;
      subrect.setLeft(subrect.left() + (substart - draw_range.in()).toDouble()*scale);
      subrect.setWidth((subend - substart).toDouble()*scale);

      rational local_start = substart - wv.in();
      AudioVisualWaveform::DrawWaveform(painter, subrect, scale, wv.waveform, local_start);
    }
  }
}

AudioVisualWaveform::Sample AudioWaveformCache::GetSummaryFromTime(const rational &start, const rational &length) const
{
  QMap<rational, AudioVisualWaveform::Sample> sample;

  TimeRange acquire(start, start+length);
  foreach (const TimeRangeWithWaveform &wv, waveforms_) {
    if (wv.OverlapsWith(acquire)) {
      TimeRange this_range = wv.Intersected(acquire);
      auto sum = wv.waveform.GetSummaryFromTime(this_range.in() - wv.in(), this_range.length());
      sample.insert(this_range.in(), sum);
    }
  }

  AudioVisualWaveform::Sample result;

  for (auto it=sample.cbegin(); it!=sample.cend(); it++) {
    result.insert(result.end(), it.value().begin(), it.value().end());
  }

  return result;
}

rational AudioWaveformCache::length() const
{
  rational len = 0;

  foreach (const TimeRangeWithWaveform &wv, waveforms_) {
    len = std::max(len, wv.out());
  }

  return len;
}

void AudioWaveformCache::SetPassthrough(PlaybackCache *cache)
{
  AudioWaveformCache *c = static_cast<AudioWaveformCache*>(cache);
  waveforms_ = c->waveforms_;
  for (const TimeRange &r : c->GetValidatedRanges()) {
    Validate(r);
  }
  SetParameters(c->GetParameters());
  SetSavingEnabled(c->IsSavingEnabled());
}

}

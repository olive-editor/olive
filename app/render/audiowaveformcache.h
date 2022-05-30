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

#ifndef AUDIOWAVEFORMCACHE_H
#define AUDIOWAVEFORMCACHE_H

#include "audio/audiovisualwaveform.h"
#include "playbackcache.h"

namespace olive {

class AudioWaveformCache : public PlaybackCache
{
  Q_OBJECT
public:
  AudioWaveformCache(QObject *parent = nullptr);

  void WriteWaveform(const TimeRange &range, const TimeRangeList &valid_ranges, const AudioVisualWaveform *waveform);

  const AudioParams &GetParameters() const { return params_; }
  void SetParameters(const AudioParams &p) { params_ = p; }

  void Draw(QPainter* painter, const QRect &rect, const double &scale, const rational &start_time) const;

  AudioVisualWaveform::Sample GetSummaryFromTime(const rational &start, const rational &length) const;

  rational length() const;

private:
  class TimeRangeWithWaveform : public TimeRange
  {
  public:
    TimeRangeWithWaveform() = default;
    TimeRangeWithWaveform(const TimeRange &r) :
      TimeRange(r)
    {
    }

    void set_in(const rational& in)
    {
      waveform.TrimIn(in - this->in());
      TimeRange::set_in(in);
    }

    void set_out(const rational& out)
    {
      waveform.Resize(out - this->in());
      TimeRange::set_out(out);
    }

    void set_range(const rational& in, const rational& out)
    {
      waveform.TrimRange(in, out-in);
      TimeRange::set_range(in, out);
    }

    AudioVisualWaveform waveform;
  };

  QVector<TimeRangeWithWaveform> waveforms_;

  AudioParams params_;

};

}

#endif // AUDIOWAVEFORMCACHE_H

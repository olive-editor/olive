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
  void SetParameters(const AudioParams &p)
  {
    params_ = p;
    waveforms_->set_channel_count(p.channel_count());
  }

  void Draw(QPainter* painter, const QRect &rect, const double &scale, const rational &start_time) const;

  AudioVisualWaveform::Sample GetSummaryFromTime(const rational &start, const rational &length) const;

  rational length() const;

  virtual void SetPassthrough(PlaybackCache *cache) override;

protected:
  virtual void InvalidateEvent(const TimeRange& range);

private:
  using WaveformPtr = std::shared_ptr<AudioVisualWaveform>;

  WaveformPtr waveforms_;

  AudioParams params_;

  class WaveformPassthrough : public TimeRange
  {
  public:
    WaveformPassthrough(const TimeRange &r) :
      TimeRange(r)
    {}

    WaveformPtr waveform;
  };

  QVector<WaveformPassthrough> passthroughs_;

};

}

#endif // AUDIOWAVEFORMCACHE_H

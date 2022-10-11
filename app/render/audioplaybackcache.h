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

#ifndef AUDIOPLAYBACKCACHE_H
#define AUDIOPLAYBACKCACHE_H

#include "audio/audiovisualwaveform.h"
#include "common/timerange.h"
#include "codec/samplebuffer.h"
#include "render/playbackcache.h"

namespace olive {

/**
 * @brief A fully integrated system of storing and playing cached audio
 *
 * All audio in Olive is processed and rendered in advance. This makes playback extremely smooth
 * and reliable, but provides some challenges as far as storing and manipulating this audio while
 * minimizing the amount of re-renders necessary.
 *
 * Olive's PlaybackCaches support "shifting"; moving cached data to a different spot on the
 * timeline without requiring a costly re-render. While video is naturally stored on disk as
 * separate frames that are easy to swap out, audio works a little differently. It would be
 * extremely inefficient to store each sample as a separate file on the disk, but storing in
 * one single contiguous file would be detrimental to shifting, particularly for longer timelines
 * since the data will actually have to be shifted on disk.
 *
 * As such, AudioPlaybackCache compromises by storing audio in several "segments". This makes
 * operations like shifting much easier since segments can simply be removed from the playlist
 * rather than having to shift or re-render potentially hours of audio in every operation.
 *
 * Naturally, storing in segments means you can't simply play the PCM data like a file, so
 * AudioPlaybackCache also provides a playback device (accessible from CreatePlaybackDevice()) that
 * acts identically to a file-based IO device, transparently joining segments together and acting
 * like one contiguous file.
 */
class AudioPlaybackCache : public PlaybackCache
{
  Q_OBJECT
public:
  AudioPlaybackCache(QObject* parent = nullptr);

  virtual ~AudioPlaybackCache() override;

  AudioParams GetParameters()
  {
    return params_;
  }

  void SetParameters(const AudioParams& params);

  void WritePCM(const TimeRange &range, const TimeRangeList &valid_ranges, const SampleBuffer &samples);

  void WriteSilence(const TimeRange &range);

private:
  bool WritePartOfSampleBuffer(const SampleBuffer &samples, const rational &write_start, const rational &buffer_start, const rational &length);

  QString GetSegmentFilename(qint64 segment_index, int channel);

  static const qint64 kDefaultSegmentSizePerChannel;

  AudioParams params_;

};

}

#endif // AUDIOPLAYBACKCACHE_H

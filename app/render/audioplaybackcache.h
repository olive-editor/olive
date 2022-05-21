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

  void WriteWaveform(const TimeRange &range, const TimeRangeList &valid_ranges, const AudioVisualWaveform *waveform);

  void WriteSilence(const TimeRange &range);

  class Segment
  {
  public:
    Segment(qint64 size = 0);

    qint64 size() const
    {
      return size_;
    }

    void set_size(qint64 sz)
    {
      size_ = sz;
    }

    qint64 offset() const
    {
      return offset_;
    }

    void set_offset(qint64 o)
    {
      offset_ = o;
    }

    int channels() const
    {
      return filenames_.size();
    }

    void set_channels(int index)
    {
      filenames_.resize(index);
    }

    const QString& filename(int index) const
    {
      return filenames_.at(index);
    }

    void set_filename(int index, const QString& filename)
    {
      filenames_[index] = filename;
    }

    qint64 end() const
    {
      return offset_ + size_;
    }

  private:
    QVector<QString> filenames_;

    qint64 size_;

    qint64 offset_;

  };

  class Playlist : public QVector<Segment>
  {
  public:
    Playlist() = default;

    int GetIndexOfPosition(qint64 pos);

    qint64 GetLength() const;

  };

  class PlaybackDevice : public QIODevice
  {
  public:
    PlaybackDevice(const Playlist& playlist, int sample_sz, QObject* parent = nullptr);

    void SetDataLimit(qint64 limit)
    {
      limit_ = limit;
    }

    virtual ~PlaybackDevice() override;

    virtual bool isSequential() const override
    {
      return false;
    }

    virtual bool seek(qint64 pos) override;

    virtual qint64 size() const override
    {
      return playlist_.GetLength();
    }

    virtual qint64 readData(char *data, qint64 maxSize) override;

    virtual qint64 writeData(const char *data, qint64 maxSize) override
    {
      Q_UNUSED(data)
      Q_UNUSED(maxSize)

      return -1;
    }

  private:
    Playlist playlist_;

    int current_segment_;

    qint64 segment_read_index_;

    int sample_size_;

    qint64 limit_;

  };

  /**
   * @brief Create a QIODevice that can play whatever's in the cache currently
   *
   * This device will act very much like a QFile, transparently linking together various segments
   * into what will appear to be a single contiguous file.
   *
   * The caller becomes responsible for ownership of the device, though the parent can be set
   * automatically as an optional parameter to this function.
   */
  PlaybackDevice* CreatePlaybackDevice(QObject *parent = nullptr) const;

  const AudioVisualWaveform &visual() const
  {
    return visual_;
  }

signals:
  void ParametersChanged();

private:
  static const qint64 kDefaultSegmentSizePerChannel;

  Segment CloneSegment(const Segment& s) const;

  Segment CreateSegment(const qint64 &size, const qint64 &offset) const;

  QString GenerateSegmentFilename() const;

  void TrimSegmentIn(Segment* s, qint64 new_length);

  void TrimSegmentOut(Segment* s, qint64 new_length);

  void RemoveSegmentFromArray(int index);

  void ClearPlaylist();

  void UpdateOffsetsFrom(int index);

  Playlist playlist_;

  AudioParams params_;

  AudioVisualWaveform visual_;

};

}

#endif // AUDIOPLAYBACKCACHE_H

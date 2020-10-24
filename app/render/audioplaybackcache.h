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

#ifndef AUDIOPLAYBACKCACHE_H
#define AUDIOPLAYBACKCACHE_H

#include "common/timerange.h"
#include "codec/samplebuffer.h"
#include "render/playbackcache.h"

OLIVE_NAMESPACE_ENTER

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

  void WritePCM(const TimeRange &range, SampleBufferPtr samples, const qint64& job_time);

  void WriteSilence(const TimeRange &range, qint64 job_time);

  QList<TimeRange> GetValidRanges(const TimeRange &range, const qint64 &job_time);

  class Segment
  {
  public:
    Segment() = default;
    Segment(qint64 size, const QString& filename);

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

    const QString& filename() const
    {
      return filename_;
    }

    void set_filename(const QString& filename)
    {
      filename_ = filename;
    }

    qint64 end() const
    {
      return offset_ + size_;
    }

  private:
    QString filename_;

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
    PlaybackDevice(const Playlist& playlist, QObject* parent = nullptr);

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

  };

  PlaybackDevice* CreatePlaybackDevice(QObject *parent = nullptr) const;

signals:
  void ParametersChanged();

protected:
  virtual void ShiftEvent(const rational& from, const rational& to) override;

  virtual void LengthChangedEvent(const rational& old, const rational& newlen) override;

private:
  static const qint64 kDefaultSegmentSize;

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

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOPLAYBACKCACHE_H

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
    Segment(const rational& length, const QString& s);

    const rational& length() const
    {
      return length_;
    }

    void set_length(const rational& length)
    {
      length_ = length;
    }

    const QString& filename() const
    {
      return filename_;
    }

    void set_filename(const QString& filename)
    {
      filename_ = filename;
    }

  private:
    QString filename_;

    rational length_;

  };

  using Playlist = QVector<Segment>;

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

    virtual qint64 size() const override;

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
  static const rational kDefaultSegmentSize;

  Segment CloneSegment(const Segment& s) const;

  Segment CreateSegment(const rational& length) const;

  QString GenerateSegmentFilename() const;

  void TrimSegmentIn(Segment* s, const rational& new_length);

  void TrimSegmentOut(Segment* s, const rational& new_length);

  void RemoveSegmentFromArray(int index);

  Playlist segments_;

  rational segment_length_;

  AudioParams params_;

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOPLAYBACKCACHE_H

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

#include "audioplaybackcache.h"

#include <QDir>
#include <QFile>
#include <QUuid>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

AudioPlaybackCache::AudioPlaybackCache()
{
  quint32 r = std::rand();
  UpdateFilename(QString::number(r));
}

void AudioPlaybackCache::SetParameters(const AudioRenderingParams &params)
{
  QMutexLocker locker(lock());

  if (params_ == params) {
    return;
  }

  params_ = params;

  // Restart empty file so there's always "something" to play
  QFile f(filename_);
  if (f.open(QFile::WriteOnly)) {
    f.close();
  }

  // Our current audio cache is unusable, so we truncate it automatically
  TimeRange invalidate_range(0, NoLockGetLength());
  NoLockInvalidate(invalidate_range);

  locker.unlock();

  emit ParametersChanged();
  emit Invalidated(invalidate_range);
}

void AudioPlaybackCache::WritePCM(const TimeRange &range, SampleBufferPtr samples, const qint64 &job_time)
{
  QMutexLocker locker(lock());

  QList<TimeRange> valid_ranges = NoLockGetValidRanges(range, job_time);
  if (valid_ranges.isEmpty()) {
    return;
  }

  QFile f(filename_);
  if (f.open(QFile::ReadWrite)) {
    QByteArray a = samples->toPackedData();

    foreach (const TimeRange& r, valid_ranges) {
      // Calculate destination offsets
      qint64 start_offset = params_.time_to_bytes(r.in());
      qint64 max_len = params_.time_to_bytes(r.out());

      if (f.size() < max_len) {
        f.resize(max_len);
      }

      // Calculate source offsets
      qint64 sample_start = params_.time_to_bytes(r.in() - range.in());
      qint64 sample_len = params_.time_to_bytes(r.length());
      qint64 actual_write = qMin(sample_len, a.size() - sample_start);

      f.seek(start_offset);
      f.write(a.data() + sample_start, actual_write);

      if (actual_write < sample_len) {
        // Fill remaining space with silence
        QByteArray s(sample_len - actual_write, 0x00);
        f.write(s);
      }
    }

    f.close();

    NoLockValidate(range);

    locker.unlock();

    emit Validated(range);
  } else {
    qWarning() << "Failed to write PCM data to" << filename_;
  }
}

void AudioPlaybackCache::WriteSilence(const TimeRange &range)
{
  QMutexLocker locker(lock());

  QFile f(filename_);
  if (f.open(QFile::ReadWrite)) {
    qint64 start_offset = params_.time_to_bytes(range.in());
    qint64 max_len = params_.time_to_bytes(range.out());
    qint64 write_len = max_len - start_offset;

    if (f.size() < max_len) {
      f.resize(max_len);
    }

    f.seek(start_offset);

    QByteArray a(write_len, 0x00);
    f.write(a);

    f.close();
  } else {
    qWarning() << "Failed to write PCM data to" << filename_;
  }
}

/*
void AudioPlaybackCache::SetUuid(const QUuid &id)
{
  UpdateFilename(id.toByteArray());
}
*/

void AudioPlaybackCache::ShiftEvent(const rational &from, const rational &to)
{
  qint64 from_offset = params_.time_to_bytes(from);
  qint64 to_offset = params_.time_to_bytes(to);

  if (from_offset == to_offset) {
    return;
  }

  QFile f(filename_);
  if (f.open(QFile::ReadWrite)) {
    if (!f.size()) {
      return;
    }

    qint64 chunk = qAbs(to_offset - from_offset);

    QByteArray buf(chunk, Qt::Uninitialized);

    if (to > from) {
      // Shifting forwards, we must insert a new region and shift all the bytes there

      // For shifting forwards, we copy bytes starting at the back so that bytes we need don't
      // get overwritten.
      qint64 read_offset = f.size();

      f.resize(f.size() + chunk);

      qint64 write_offset = f.size();

      while (read_offset != from_offset) {

        // Calculate how much will be read this time
        qint64 chunk_sz = qMin(chunk, read_offset - from_offset);
        read_offset -= chunk_sz;

        // Read that chunk
        f.seek(read_offset);
        f.read(buf.data(), chunk_sz);

        // Write it at the destination
        write_offset -= chunk_sz;
        f.seek(write_offset);
        f.write(buf.data(), chunk_sz);

      }

      // Replace remainder with silence
      f.seek(from_offset);
      buf.fill(0);
      f.write(buf);

    } else {
      // Shifting backwards, we will shift bytes and truncate

      while (from_offset != f.size()) {
        // Read region to be shifted
        f.seek(from_offset);
        qint64 read_sz = f.read(buf.data(), buf.size());
        from_offset += read_sz;

        // Write it at the destination
        f.seek(to_offset);
        to_offset += f.write(buf, read_sz);
      }

      // Truncate
      f.resize(f.size() - chunk);

    }

    f.close();
  } else {
    qWarning() << "Failed to write PCM data to" << filename_;
  }
}

void AudioPlaybackCache::LengthChangedEvent(const rational& old, const rational& newlen)
{
  if (newlen < old) {
    QFile(filename_).resize(params_.time_to_bytes(newlen));
  }
}

QList<TimeRange> AudioPlaybackCache::NoLockGetValidRanges(const TimeRange& range, const qint64& job_time)
{
  QList<TimeRange> valid_ranges;

  for (int i=jobs_.size()-1;i>=0;i--) {
    const JobIdentifier& job = jobs_.at(i);

    if (job_time >= job.job_time && job.range.OverlapsWith(range)) {
      valid_ranges.append(job.range.Intersected(range));
    }
  }

  return valid_ranges;
}

void AudioPlaybackCache::UpdateFilename(const QString &s)
{
  filename_ = QDir(FileFunctions::GetMediaCacheLocation()).filePath(s);
  filename_.append(QStringLiteral(".pcm"));
}

const QString &AudioPlaybackCache::GetCacheFilename() const
{
  return filename_;
}

OLIVE_NAMESPACE_EXIT

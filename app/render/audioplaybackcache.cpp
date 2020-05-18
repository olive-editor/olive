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
#include <QRandomGenerator>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

AudioPlaybackCache::AudioPlaybackCache()
{
  // FIXME: We just use a randomly generated number, one day this should be matchable to the
  //        project so it can be reused.
  quint32 r = QRandomGenerator::global()->generate();
  filename_ = QDir(FileFunctions::GetMediaCacheLocation()).filePath(QString::number(r));
  filename_.append(QStringLiteral(".pcm"));
}

void AudioPlaybackCache::SetParameters(const AudioRenderingParams &params)
{
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
  InvalidateAll();

  emit ParametersChanged();
}

void AudioPlaybackCache::WritePCM(const TimeRange &range, SampleBufferPtr samples)
{
  QFile f(filename_);
  qDebug() << "Writing PCM to" << filename_;
  if (f.open(QFile::ReadWrite)) {
    qint64 start_offset = params_.time_to_bytes(range.in());
    qint64 max_len = params_.time_to_bytes(range.out());
    qint64 write_len = max_len - start_offset;

    QByteArray a = samples->toPackedData();

    if (f.size() < max_len) {
      f.resize(max_len);
    }

    f.seek(start_offset);
    f.write(a);

    if (write_len > a.size()) {
      // Fill remaining space with silence
      QByteArray s(write_len - a.size(), 0x00);
      f.write(s);
    }

    f.close();
  } else {
    qWarning() << "Failed to write PCM data to" << filename_;
  }
}

void AudioPlaybackCache::WriteSilence(const TimeRange &range)
{
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

const QString &AudioPlaybackCache::GetCacheFilename() const
{
  return filename_;
}

OLIVE_NAMESPACE_EXIT

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

const QString &AudioPlaybackCache::GetCacheFilename() const
{
  return filename_;
}

OLIVE_NAMESPACE_EXIT

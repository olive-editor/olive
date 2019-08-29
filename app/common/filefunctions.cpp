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

#include "filefunctions.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QString GetUniqueFileIdentifier(const QString &filename)
{
  QFileInfo info(filename);

  if (!info.exists()) {
    return QString();
  }

  QCryptographicHash hash(QCryptographicHash::Sha1);

  hash.addData(info.absoluteFilePath().toUtf8());

  hash.addData(info.lastModified().toString().toUtf8());

  QByteArray result = hash.result();

  return QString(result.toHex());
}

QString GetMediaIndexLocation()
{
  QDir local_appdata_dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));

  QDir media_index_dir = local_appdata_dir.filePath("mediaindex");

  // Attempt to ensure this folder exists
  media_index_dir.mkpath(".");

  return media_index_dir.absolutePath();
}

QString GetMediaIndexFilename(const QString &filename)
{
  return QDir(GetMediaIndexLocation()).filePath(filename);
}

QString GetMediaCacheLocation()
{
  QDir local_appdata_dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));

  QDir media_cache_dir = local_appdata_dir.filePath("mediacache");

  // Attempt to ensure this folder exists
  media_cache_dir.mkpath(".");

  return media_cache_dir.absolutePath();
}

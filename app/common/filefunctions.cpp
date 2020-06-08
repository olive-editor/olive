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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include "config/config.h"

OLIVE_NAMESPACE_ENTER

QString FileFunctions::GetUniqueFileIdentifier(const QString &filename)
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

QString FileFunctions::GetMediaIndexLocation()
{
  QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());

  QDir media_index_dir = local_appdata_dir.filePath("mediaindex");

  // Attempt to ensure this folder exists
  media_index_dir.mkpath(".");

  return media_index_dir.absolutePath();
}

QString FileFunctions::GetMediaIndexFilename(const QString &filename)
{
  return QDir(GetMediaIndexLocation()).filePath(filename);
}

QString FileFunctions::GetMediaCacheLocation()
{
  QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());

  QDir media_cache_dir = local_appdata_dir.filePath("mediacache");

  // Attempt to ensure this folder exists
  media_cache_dir.mkpath(".");

  return media_cache_dir.absolutePath();
}

QString FileFunctions::GetConfigurationLocation()
{
  if (IsPortable()) {
    return GetApplicationPath();
  } else {
    QString s = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir(s).mkpath(".");
    return s;
  }
}

bool FileFunctions::IsPortable()
{
  return QFileInfo::exists(QDir(GetApplicationPath()).filePath("portable"));
}

QString FileFunctions::GetApplicationPath()
{
  return QCoreApplication::applicationDirPath();
}

QString FileFunctions::GetTempFilePath()
{
  QString temp_path = QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                           .filePath(QCoreApplication::organizationName()))
                   .filePath(QCoreApplication::applicationName());

  // Ensure it exists
  QDir(temp_path).mkpath(".");

  return temp_path;
}

void FileFunctions::CopyDirectory(const QString &source, const QString &dest, bool overwrite)
{
  QDir d(source);

  if (!d.exists()) {
    qCritical() << "Failed to copy directory, source" << source << "didn't exist";
    return;
  }

  QDir dest_dir(dest);

  if (!dest_dir.mkpath(QStringLiteral("."))) {
    qCritical() << "Failed to create destination directory" << dest;
    return;
  }

  QFileInfoList l = d.entryInfoList();

  foreach (const QFileInfo& info, l) {
    // QDir::NoDotAndDotDot continues to not work, so we have to check manually
    if (info.fileName() == QStringLiteral(".") || info.fileName() == QStringLiteral("..")) {
      continue;
    }

    QString dest_file_path = dest_dir.filePath(info.fileName());

    if (info.isDir()) {
      // Copy dir
      CopyDirectory(info.absoluteFilePath(), dest_file_path, overwrite);
    } else {
      // Copy file
      if (overwrite && QFile::exists(dest_file_path)) {
        QFile::remove(dest_file_path);
      }

      QFile::copy(info.absoluteFilePath(), dest_file_path);
    }
  }
}

OLIVE_NAMESPACE_EXIT

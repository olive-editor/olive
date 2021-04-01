/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

QString FileFunctions::GetUniqueFileIdentifier(const QString &filename)
{
  QFileInfo info(filename);

  if (!info.exists()) {
    return QString();
  }

  QCryptographicHash hash(QCryptographicHash::Sha1);

  hash.addData(info.absoluteFilePath().toUtf8());

  hash.addData(QString::number(info.lastModified().toMSecsSinceEpoch()).toUtf8());

  QByteArray result = hash.result();

  return QString(result.toHex());
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

bool FileFunctions::CanCopyDirectoryWithoutOverwriting(const QString& source, const QString& dest)
{
  QFileInfoList info_list = QDir(source).entryInfoList();

  foreach (const QFileInfo& info, info_list) {
    // QDir::NoDotAndDotDot continues to not work, so we have to check manually
    if (info.fileName() == QStringLiteral(".") || info.fileName() == QStringLiteral("..")) {
      continue;
    }

    QString dest_equivalent = QDir(dest).filePath(info.fileName());

    if (info.isDir()) {
      if (!CanCopyDirectoryWithoutOverwriting(info.absoluteFilePath(), dest_equivalent)) {
        return false;
      }
    } else if (QFileInfo::exists(dest_equivalent)) {
      return false;
    }
  }

  return true;
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

bool FileFunctions::DirectoryIsValid(const QString &dir, bool try_to_create)
{
  // Empty string is invalid
  if (dir.isEmpty()) {
    return false;
  }

  QDir d(dir);

  // If directory already exists, this is valid
  if (d.exists()) {
    return true;
  }

  // If we can create and creation is successful, this is valid
  if (try_to_create && d.mkpath(".")) {
    return true;
  }

  // Otherwise, invalid
  return false;
}

QString FileFunctions::EnsureFilenameExtension(QString fn, const QString &extension)
{
  // No-op if either input is empty
  if (!fn.isEmpty() && !extension.isEmpty()) {
    QString extension_with_dot;

    extension_with_dot.append('.');
    extension_with_dot.append(extension);

    if (!fn.endsWith(extension_with_dot, Qt::CaseInsensitive)) {
      fn.append(extension_with_dot);
    }
  }

  return fn;
}

QString FileFunctions::ReadFileAsString(const QString &filename)
{
  QFile f(filename);
  QString file_data;
  if (f.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream text_stream(&f);
    file_data = text_stream.readAll();
    f.close();
  }
  return file_data;
}

QString FileFunctions::GetSafeTemporaryFilename(const QString &original)
{
  int counter = 0;

  QFileInfo original_info(original);
  QString basename = original_info.baseName();
  QString complete_suffix = original_info.completeSuffix();

  // If we have a complete suffix, make sure there's a period in it
  if (!complete_suffix.isEmpty()) {
    complete_suffix.prepend('.');
  }

  QString temp_abs_path;
  do {
    temp_abs_path = original_info.dir().filePath(
          QStringLiteral("%1.tmp%2%3").arg(basename,
                                           QString::number(counter),
                                           complete_suffix));
    counter++;
  } while (QFileInfo::exists(temp_abs_path));

  return temp_abs_path;
}

bool FileFunctions::RenameFileAllowOverwrite(const QString &from, const QString &to)
{
  if (QFileInfo::exists(to) && !QFile::remove(to)) {
    qCritical() << "Couldn't remove existing file" << to << "for overwrite";
    return false;
  }

  // By this point, we can assume `to` either never existed or has now been deleted
  if (!QFile::rename(from, to)) {
    qCritical() << "Failed to rename file" << from << "to" << to;
    return false;
  }

  return true;
}

QString FileFunctions::GetFormattedExecutableForPlatform(QString unformatted)
{
#ifdef Q_OS_WINDOWS
  unformatted.append(QStringLiteral(".exe"));
#endif

  return unformatted;
}

}

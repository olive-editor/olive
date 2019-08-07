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
  media_index_dir.mkpath("mediaindex");

  return media_index_dir.absolutePath();
}

QString GetMediaIndexFilename(const QString &filename)
{
  return QDir(GetMediaIndexLocation()).filePath(filename);
}

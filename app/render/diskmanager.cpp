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

#include "diskmanager.h"

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "common/filefunctions.h"
#include "config/config.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

DiskManager* DiskManager::instance_ = nullptr;

DiskManager::DiskManager()
{
  // Add default cache location
  QFile default_disk_cache_file(QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("defaultdiskcache")));
  if (default_disk_cache_file.open(QFile::ReadOnly)) {
    QString default_dir = default_disk_cache_file.readAll();

    if (FileFunctions::DirectoryIsValid(default_dir, true)) {
      GetOpenFolder(default_dir);
    } else {
      QMessageBox::warning(nullptr,
                           tr("Disk Cache Error"),
                           tr("Unable to set custom application disk cache. Using default instead."));
    }

    default_disk_cache_file.close();
  }

  // If no custom default was loaded, load default
  if (open_folders_.isEmpty()) {
    GetOpenFolder(QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("mediacache"));
  }

  QFile disk_cache_index(QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("diskcache")));

  if (disk_cache_index.open(QFile::ReadOnly)) {
    QTextStream stream(&disk_cache_index);

    QString line;
    while (stream.readLineInto(&line)) {
      GetOpenFolder(line);
    }

    disk_cache_index.close();
  }
}

DiskManager::~DiskManager()
{
}

void DiskManager::CreateInstance()
{
  instance_ = new DiskManager();
}

void DiskManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
}

DiskManager *DiskManager::instance()
{
  return instance_;
}

void DiskManager::Accessed(const QString &cache_folder, const QByteArray &hash)
{
  DiskCacheFolder* f = GetOpenFolder(cache_folder);

  f->Accessed(hash);
}

void DiskManager::CreatedFile(const QString &cache_folder, const QString &file_name, const QByteArray &hash)
{
  DiskCacheFolder* f = GetOpenFolder(cache_folder);

  f->CreatedFile(file_name, hash);
}

bool DiskManager::ClearDiskCache(const QString &cache_folder)
{
  DiskCacheFolder* f = GetOpenFolder(cache_folder);

  return f->ClearCache();
}

DiskCacheFolder *DiskManager::GetOpenFolder(const QString &path)
{
  // If path is empty, this must mean default
  if (path.isEmpty()) {
    return GetDefaultCacheFolder();
  }

  // See if we have an existing path with this name
  foreach (DiskCacheFolder* f, open_folders_) {
    if (f->GetPath() == path) {
      return f;
    }
  }

  // We must have to open this folder
  DiskCacheFolder* f = new DiskCacheFolder(path, this);
  connect(f, &DiskCacheFolder::DeletedFrame, this, &DiskManager::DeletedFrame);
  open_folders_.append(f);

  return f;
}

bool DiskManager::ShowDiskCacheChangeConfirmationDialog(QWidget *parent)
{
  return (QMessageBox::question(parent,
                                tr("Disk Cache"),
                                tr("You've chosen to change the default disk cache location. This "
                                   "will invalidate your current cache. Would you like to continue?"),
                                QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
}

DiskCacheFolder::DiskCacheFolder(const QString &path, QObject *parent) :
  QObject(parent)
{
  SetPath(path);
}

DiskCacheFolder::~DiskCacheFolder()
{
  CloseCacheFolder();
}

bool DiskCacheFolder::ClearCache()
{
  bool deleted_files = true;

  std::list<HashTime>::iterator i = disk_data_.begin();

  while (i != disk_data_.end()) {
    // We return a false result if any of the files fail to delete, but still try to delete as many as we can
    if (QFile::remove(i->file_name) || !QFileInfo::exists(i->file_name)) {
      emit DeletedFrame(path_, i->hash);
      i = disk_data_.erase(i);
    } else {
      qWarning() << "Failed to delete" << i->file_name;
      deleted_files = false;
      i++;
    }
  }

  return deleted_files;
}

void DiskCacheFolder::Accessed(const QByteArray &hash)
{
  std::list<HashTime>::iterator i = disk_data_.begin();

  while (i != disk_data_.end()) {
    if (i->hash == hash) {
      // Copy access data and erase from list
      HashTime accessed_hash = *i;
      disk_data_.erase(i);

      // Add it to the end
      disk_data_.push_back(accessed_hash);

      // End loop
      break;
    } else {
      i++;
    }
  }
}

void DiskCacheFolder::CreatedFile(const QString &file_name, const QByteArray &hash)
{
  qint64 file_size = QFile(file_name).size();

  disk_data_.push_back({file_name, hash, file_size});

  consumption_ += file_size;

  QList<QByteArray> deleted_hashes;

  while (consumption_ > limit_) {
    deleted_hashes.append(DeleteLeastRecent());
  }

  foreach (const QByteArray& h, deleted_hashes) {
    emit DeletedFrame(path_, h);
  }
}

void DiskCacheFolder::SetPath(const QString &path)
{
  // If this is currently set to a folder, close it out now
  CloseCacheFolder();

  // Signal that disk cache is gone
  if (!disk_data_.empty()) {
    foreach (const HashTime& h, disk_data_) {
      emit DeletedFrame(path_, h.hash);
    }
    disk_data_.clear();
  }

  // Set defaults
  clear_on_close_ = false;
  consumption_ = 0;
  limit_ = 21474836480; // Default to 20 GB

  // Set path
  path_ = path;

  // Attempt to load existing index file from path
  QDir path_dir(path_);
  path_dir.mkpath(".");

  index_path_ = path_dir.filePath(QStringLiteral("index"));

  // Try to load any current cache index from file
  QFile cache_index_file(index_path_);

  if (cache_index_file.open(QFile::ReadOnly)) {
    QDataStream ds(&cache_index_file);

    ds >> limit_;
    ds >> clear_on_close_;

    while (!cache_index_file.atEnd()) {
      HashTime h;

      ds >> h.file_name;
      ds >> h.hash;
      ds >> h.file_size;

      if (QFileInfo::exists(h.file_name)) {
        consumption_ += h.file_size;
        disk_data_.push_back(h);
      }
    }

    cache_index_file.close();
  }
}

QByteArray DiskCacheFolder::DeleteLeastRecent()
{
  HashTime h = disk_data_.front();
  disk_data_.pop_front();

  QFile::remove(h.file_name);

  consumption_ -= h.file_size;

  return h.hash;
}

void DiskCacheFolder::CloseCacheFolder()
{
  if (path_.isEmpty()) {
    return;
  }

  if (clear_on_close_) {
    // If we're not moving to new and we're set to clear on close, clear now or else it'll never
    // get cleared later
    ClearCache();
  }

  // Save current cache index
  QFile cache_index_file(index_path_);

  if (cache_index_file.open(QFile::WriteOnly)) {
    QDataStream ds(&cache_index_file);

    ds << limit_;
    ds << clear_on_close_;

    foreach (const HashTime& h, disk_data_) {
      ds << h.file_name;
      ds << h.hash;
      ds << h.file_size;
    }

    cache_index_file.close();
  } else {
    qWarning() << "Failed to write cache index:" << index_path_;
  }
}

OLIVE_NAMESPACE_EXIT

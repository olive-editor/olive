/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "common/filefunctions.h"
#include "config/config.h"
#include "core.h"
#include "dialog/diskcache/diskcachedialog.h"

namespace olive {

DiskManager* DiskManager::instance_ = nullptr;

DiskManager::DiskManager()
{
  // Add default cache location
  QFile default_disk_cache_file(GetDefaultDiskCacheConfigFile());
  if (default_disk_cache_file.open(QFile::ReadOnly)) {
    QString default_dir = default_disk_cache_file.readAll();

    if (!default_dir.isEmpty()) {
      if (FileFunctions::DirectoryIsValid(default_dir)) {
        GetOpenFolder(default_dir);
      } else {
        QMessageBox::warning(nullptr,
                             tr("Disk Cache Error"),
                             tr("Unable to set custom application disk cache. Using default instead."));
      }
    }

    default_disk_cache_file.close();
  }

  // If no custom default was loaded, load default
  if (open_folders_.isEmpty()) {
    GetOpenFolder(GetDefaultDiskCachePath());
  }

  QFile disk_cache_index(QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("diskcache2")));

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
  QFile default_disk_cache_file(GetDefaultDiskCacheConfigFile());
  if (default_disk_cache_file.open(QFile::WriteOnly)) {
    if (GetDefaultDiskCachePath() != GetDefaultCachePath()) {
      default_disk_cache_file.write(GetDefaultCachePath().toUtf8());
    }

    default_disk_cache_file.close();
  }
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

void DiskManager::Accessed(const QString &cache_folder, const QString &filename)
{
  DiskCacheFolder* f = GetOpenFolder(cache_folder);

  f->Accessed(filename);
}

void DiskManager::CreatedFile(const QString &cache_folder, const QString &filename)
{
  DiskCacheFolder* f = GetOpenFolder(cache_folder);

  f->CreatedFile(filename);
}

void DiskManager::DeleteSpecificFile(const QString &filename)
{
  foreach (DiskCacheFolder* f, open_folders_) {
    f->DeleteSpecificFile(filename);
  }
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

QString DiskManager::GetDefaultDiskCacheConfigFile()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("defaultdiskcache"));
}

QString DiskManager::GetDefaultDiskCachePath()
{
  return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("mediacache");
}

void DiskManager::ShowDiskCacheSettingsDialog(DiskCacheFolder *folder, QWidget *parent)
{
  DiskCacheDialog d(folder, parent);
  d.exec();
}

void DiskManager::ShowDiskCacheSettingsDialog(const QString &path, QWidget *parent)
{
  if (!FileFunctions::DirectoryIsValid(path)) {
    QMessageBox::critical(parent, tr("Disk Cache Error"),
                          tr("Failed to open disk cache at \"%1\". Try a different folder.").arg(path));
    return;
  }

  DiskCacheFolder* folder = GetOpenFolder(path);

  ShowDiskCacheSettingsDialog(folder, parent);
}

DiskCacheFolder::DiskCacheFolder(const QString &path, QObject *parent) :
  QObject(parent)
{
  SetPath(path);

  save_timer_.setInterval(OLIVE_CONFIG("DiskCacheSaveInterval").toInt());
  connect(&save_timer_, &QTimer::timeout, this, &DiskCacheFolder::SaveDiskCacheIndex);
  save_timer_.start();
}

DiskCacheFolder::~DiskCacheFolder()
{
  CloseCacheFolder();
}

bool DiskCacheFolder::ClearCache()
{
  bool deleted_files = true;

  auto i = disk_data_.begin();

  while (i != disk_data_.end()) {
    // We return a false result if any of the files fail to delete, but still try to delete as many as we can
    QString filename = i.key();

    if (QFile::remove(filename) || !QFileInfo::exists(filename)) {
      emit DeletedFrame(path_, filename);
      i = disk_data_.erase(i);
    } else {
      qWarning() << "Failed to delete" << filename;
      deleted_files = false;
      i++;
    }
  }

  return deleted_files;
}

void DiskCacheFolder::Accessed(const QString &filename)
{
  if (!disk_data_.contains(filename)) {
    return;
  }

  disk_data_[filename].access_time = QDateTime::currentMSecsSinceEpoch();
}

void DiskCacheFolder::CreatedFile(const QString &filename)
{
  qint64 file_size = QFile(filename).size();

  disk_data_.insert(filename, {file_size, QDateTime::currentMSecsSinceEpoch()});

  consumption_ += file_size;

  while (consumption_ > limit_) {
    DeleteLeastRecent();
  }
}

void DiskCacheFolder::SetPath(const QString &path)
{
  // If this is currently set to a folder, close it out now
  CloseCacheFolder();

  // Signal that disk cache is gone
  if (!disk_data_.empty()) {
    for (auto it=disk_data_.cbegin(); it!=disk_data_.cend(); it++) {
      emit DeletedFrame(path_, it.key());
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
  FileFunctions::DirectoryIsValid(path_dir);

  index_path_ = path_dir.filePath(QStringLiteral("index"));

  // Try to load any current cache index from file
  QFile cache_index_file(index_path_);

  if (cache_index_file.open(QFile::ReadOnly)) {
    QDataStream ds(&cache_index_file);

    ds >> limit_;
    ds >> clear_on_close_;

    while (!cache_index_file.atEnd()) {
      QString filename;
      HashTime h;

      ds >> filename;
      ds >> h.file_size;
      ds >> h.access_time;

      if (QFileInfo::exists(filename)) {
        consumption_ += h.file_size;
        disk_data_.insert(filename, h);
      }
    }

    cache_index_file.close();
  }
}

bool DiskCacheFolder::DeleteFileInternal(QMap<QString, HashTime>::iterator hash_to_delete)
{
  // Cache HashTime object
  QString filename = hash_to_delete.key();
  HashTime ht = hash_to_delete.value();

  // Remove from disk
  QFile f(filename);

  if (!f.exists() || f.remove()) {
    // Remove from internal map
    disk_data_.erase(hash_to_delete);

    // Reduce consumption
    consumption_ -= ht.file_size;

    emit DeletedFrame(path_, filename);
    return true;
  }

  return false;
}

bool DiskCacheFolder::DeleteSpecificFile(const QString &f)
{
  for (auto it=disk_data_.begin(); it!=disk_data_.end(); it++) {
    if (it.key() == f) {
      // Break out of this loop, assuming we'll only have one instance of each filename
      return DeleteFileInternal(it);
    }
  }

  return false;
}

bool DiskCacheFolder::DeleteLeastRecent()
{
  auto hash_to_delete = disk_data_.begin();

  if (disk_data_.begin() != disk_data_.end()) {
    for (auto it=disk_data_.begin()+1; it!=disk_data_.end(); it++) {
      if (it->access_time < hash_to_delete->access_time) {
        hash_to_delete = it;
      }
    }

    bool e = DeleteFileInternal(hash_to_delete);

    if (e) {
      Core::instance()->WarnCacheFull();
    }

    return e;
  } else {
    return false;
  }
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
  SaveDiskCacheIndex();
}

void DiskCacheFolder::SaveDiskCacheIndex()
{
  QFile cache_index_file(index_path_);

  if (cache_index_file.open(QFile::WriteOnly)) {
    QDataStream ds(&cache_index_file);

    ds << limit_;
    ds << clear_on_close_;

    for (auto it=disk_data_.cbegin(); it!=disk_data_.cend(); it++) {
      const HashTime& ht = it.value();

      ds << it.key();
      ds << ht.file_size;
      ds << ht.access_time;
    }

    cache_index_file.close();
  } else {
    qWarning() << "Failed to write cache index:" << index_path_;
  }
}

}

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

#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>

#include "common/define.h"
#include "project/project.h"

OLIVE_NAMESPACE_ENTER

class DiskCacheFolder : public QObject
{
  Q_OBJECT
public:
  DiskCacheFolder(const QString& path, QObject* parent = nullptr);

  virtual ~DiskCacheFolder() override;

  bool ClearCache();

  void Accessed(const QByteArray& hash);

  void CreatedFile(const QString& file_name, const QByteArray& hash);

  const QString& GetPath() const
  {
    return path_;
  }

  void SetPath(const QString& path);

  qint64 GetLimit() const
  {
    return limit_;
  }

  bool GetClearOnClose() const
  {
    return clear_on_close_;
  }

  void SetLimit(qint64 l)
  {
    limit_ = l;
  }

  void SetClearOnClose(bool e)
  {
    clear_on_close_ = e;
  }

signals:
  void DeletedFrame(const QString& path, const QByteArray& hash);

private:
  QByteArray DeleteLeastRecent();

  void CloseCacheFolder();

  QString path_;

  QString index_path_;

  struct HashTime {
    QString file_name;
    QByteArray hash;
    qint64 file_size;
  };

  std::list<HashTime> disk_data_;

  qint64 consumption_;

  qint64 limit_;

  bool clear_on_close_;

  QTimer save_timer_;

private slots:
  void SaveDiskCacheIndex();

};

class DiskManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance();

  static void DestroyInstance();

  static DiskManager* instance();

  bool ClearDiskCache(const QString& cache_folder);

  DiskCacheFolder* GetDefaultCacheFolder() const
  {
    // The first folder will always be the default
    return open_folders_.first();
  }

  const QString& GetDefaultCachePath() const
  {
    return GetDefaultCacheFolder()->GetPath();
  }

  DiskCacheFolder* GetOpenFolder(const QString& path);

  const QVector<DiskCacheFolder*>& GetOpenFolders() const
  {
    return open_folders_;
  }

  static bool ShowDiskCacheChangeConfirmationDialog(QWidget* parent);

  static QString GetDefaultDiskCacheConfigFile();

  static QString GetDefaultDiskCachePath();

  void ShowDiskCacheSettingsDialog(DiskCacheFolder* folder, QWidget* parent);
  void ShowDiskCacheSettingsDialog(const QString& path, QWidget* parent);

public slots:
  void Accessed(const QString& cache_folder, const QByteArray& hash);

  void CreatedFile(const QString& cache_folder, const QString& file_name, const QByteArray& hash);

signals:
  void DeletedFrame(const QString& path, const QByteArray& hash);

  void InvalidateProject(Project* p);

private:
  DiskManager();

  virtual ~DiskManager() override;

  static DiskManager* instance_;

  QVector<DiskCacheFolder*> open_folders_;

};

OLIVE_NAMESPACE_EXIT

#endif // DISKMANAGER_H

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

#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QMutex>
#include <QObject>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class DiskManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance();

  static void DestroyInstance();

  static DiskManager* instance();

  void Accessed(const QByteArray& hash);

  void Accessed(const QString& filename);

  void CreatedFile(const QString& file_name, const QByteArray& hash);

  bool ClearDiskCache(bool quick_delete);

signals:
  void DeletedFrame(const QByteArray& hash);

private:
  DiskManager();

  virtual ~DiskManager() override;

  static DiskManager* instance_;

  QByteArray DeleteLeastRecent();

  qint64 DiskLimit();

  static QString GetCacheIndexFilename();

  struct HashTime {
    QString file_name;
    QByteArray hash;
    qint64 access_time;
    qint64 file_size;
  };

  QList<HashTime> disk_data_;

  qint64 consumption_;

  QMutex lock_;

};

OLIVE_NAMESPACE_EXIT

#endif // DISKMANAGER_H

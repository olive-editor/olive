#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <QMutex>
#include <QObject>

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

  bool ClearDiskCache();

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

#endif // DISKMANAGER_H

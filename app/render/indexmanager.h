#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <QObject>

#include "project/item/footage/stream.h"
#include "task/index/index.h"

class IndexManager : public QObject
{
  Q_OBJECT
public:
  IndexManager();

  static void CreateInstance();
  static IndexManager* instance();
  static void DestroyInstance();

  bool IsIndexing(StreamPtr stream);

public slots:
  void StartIndexingStream(StreamPtr stream);

signals:
  void StreamIndexUpdated(Stream* stream);

private:
  static IndexManager* instance_;

  struct StreamThreadPair {
    StreamPtr stream;
    IndexTask* task;
  };

  QList<StreamThreadPair> threads_;

private slots:
  void IndexTaskFinished();

  void StreamIndexUpdatedEvent();

};

#endif // INDEXMANAGER_H

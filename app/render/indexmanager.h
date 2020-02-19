#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <QObject>

#include "project/item/footage/stream.h"
#include "task/conform/conform.h"
#include "task/index/index.h"

class IndexManager : public QObject
{
  Q_OBJECT
public:
  IndexManager();

  static void CreateInstance();
  static IndexManager* instance();
  static void DestroyInstance();

  bool IsIndexing(StreamPtr stream) const;
  bool IsConforming(AudioStreamPtr stream, const AudioRenderingParams& params) const;

public slots:
  void StartIndexingStream(StreamPtr stream);
  void StartConformingStream(AudioStreamPtr stream, const AudioRenderingParams& params);

signals:
  void StreamIndexUpdated(Stream* stream);
  void StreamConformAppended(Stream* stream, const AudioRenderingParams& params);

private:
  static IndexManager* instance_;

  struct IndexPair {
    StreamPtr stream;
    IndexTask* task;
  };

  struct ConformPair {
    StreamPtr stream;
    AudioRenderingParams params;
    ConformTask* task;
  };

  QList<IndexPair> indexing_;

  QList<ConformPair> conforming_;

private slots:
  void IndexTaskFinished();

  void StreamIndexUpdatedEvent();

  void StreamConformAppendedEvent(const AudioRenderingParams& params);

};

#endif // INDEXMANAGER_H

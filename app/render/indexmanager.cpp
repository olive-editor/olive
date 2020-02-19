#include "indexmanager.h"

#include "task/taskmanager.h"

IndexManager* IndexManager::instance_ = nullptr;

IndexManager::IndexManager()
{
}

void IndexManager::CreateInstance()
{
  instance_ = new IndexManager();
}

IndexManager *IndexManager::instance()
{
  return instance_;
}

void IndexManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
}

void IndexManager::StartIndexingStream(StreamPtr stream)
{
  if (IsIndexing(stream)) {
    return;
  }

  IndexTask* index_task = new IndexTask(stream);
  indexing_.append({stream, index_task});

  connect(stream.get(), &Stream::IndexChanged, this, &IndexManager::StreamIndexUpdatedEvent, Qt::QueuedConnection);
  connect(index_task, &IndexTask::Succeeded, this, &IndexManager::IndexTaskFinished, Qt::QueuedConnection);

  TaskManager::instance()->AddTask(index_task);
}

void IndexManager::StartConformingStream(AudioStreamPtr stream, const AudioRenderingParams &params)
{
  if (IsConforming(stream, params)) {
    return;
  }

  ConformTask* conform_task = new ConformTask(stream, params);
  conforming_.append({stream, params, conform_task});

  connect(stream.get(), &AudioStream::ConformAppended, this, &IndexManager::StreamConformAppendedEvent, Qt::QueuedConnection);
  connect(conform_task, &ConformTask::Succeeded, this, &IndexManager::IndexTaskFinished, Qt::QueuedConnection);

  TaskManager::instance()->AddTask(conform_task);
}

bool IndexManager::IsIndexing(StreamPtr stream) const
{
  foreach (const IndexPair& stp, indexing_) {
    if (stp.stream == stream) {
      return true;
    }
  }

  return false;
}

bool IndexManager::IsConforming(AudioStreamPtr stream, const AudioRenderingParams &params) const
{
  foreach (const ConformPair& cfp, conforming_) {
    if (cfp.stream == stream && cfp.params == params) {
      return true;
    }
  }

  return false;
}

void IndexManager::IndexTaskFinished()
{
  for (int i=0;i<indexing_.size();i++) {
    const IndexPair& stp = indexing_.at(i);

    if (stp.task == sender()) {
      //emit StreamIndexUpdated(stp.stream.get());
      indexing_.removeAt(i);
      return;
    }
  }
}

void IndexManager::StreamIndexUpdatedEvent()
{
  emit StreamIndexUpdated(static_cast<Stream*>(sender()));
}

void IndexManager::StreamConformAppendedEvent(const AudioRenderingParams &params)
{
  qDebug() << "Got stream conform appended event";
  emit StreamConformAppended(static_cast<Stream*>(sender()), params);
}

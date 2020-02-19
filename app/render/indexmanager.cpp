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
  threads_.append({stream, index_task});

  connect(stream.get(), &Stream::IndexChanged, this, &IndexManager::StreamIndexUpdatedEvent, Qt::QueuedConnection);
  connect(index_task, &IndexTask::Succeeded, this, &IndexManager::IndexTaskFinished, Qt::QueuedConnection);

  TaskManager::instance()->AddTask(index_task);
}

bool IndexManager::IsIndexing(StreamPtr stream)
{
  foreach (const StreamThreadPair& stp, threads_) {
    if (stp.stream == stream) {
      return true;
    }
  }

  return false;
}

void IndexManager::IndexTaskFinished()
{
  for (int i=0;i<threads_.size();i++) {
    const StreamThreadPair& stp = threads_.at(i);

    if (stp.task == sender()) {
      //emit StreamIndexUpdated(stp.stream.get());
      threads_.removeAt(i);
      return;
    }
  }
}

void IndexManager::StreamIndexUpdatedEvent()
{
  emit StreamIndexUpdated(static_cast<Stream*>(sender()));
}

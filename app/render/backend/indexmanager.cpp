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

#include "indexmanager.h"

#include "task/taskmanager.h"

OLIVE_NAMESPACE_ENTER

IndexManager* IndexManager::instance_ = nullptr;

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

void IndexManager::StartConformingStream(AudioStreamPtr stream, AudioRenderingParams params)
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
  emit StreamConformAppended(static_cast<Stream*>(sender()), params);
}

OLIVE_NAMESPACE_EXIT

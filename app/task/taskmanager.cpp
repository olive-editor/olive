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

#include "taskmanager.h"

#include <QDebug>
#include <QThread>

OLIVE_NAMESPACE_ENTER

TaskManager* TaskManager::instance_ = nullptr;

TaskManager::TaskManager()
{
}

TaskManager::~TaskManager()
{
  thread_pool_.clear();

  foreach (Task* t, tasks_) {
    t->Cancel();
  }

  thread_pool_.waitForDone();

  foreach (Task* t, tasks_) {
    t->deleteLater();
  }
}

void TaskManager::CreateInstance()
{
  instance_ = new TaskManager();
}

void TaskManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
}

TaskManager *TaskManager::instance()
{
  return instance_;
}

int TaskManager::GetTaskCount() const
{
  return tasks_.size();
}

Task *TaskManager::GetFirstTask() const
{
  return tasks_.begin().value();
}

void TaskManager::CancelTaskAndWait(Task* t)
{
  t->Cancel();

  QFutureWatcher<bool>* w = tasks_.key(t);

  if (w) {
    w->waitForFinished();
  }
}

void TaskManager::AddTask(Task* t)
{
  // Create a watcher for signalling
  QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>();
  connect(watcher, &QFutureWatcher<bool>::finished, this, &TaskManager::TaskFinished);

  // Add the Task to the queue
  tasks_.insert(watcher, t);

  // Run task concurrently
  watcher->setFuture(QtConcurrent::run(t, &Task::Start));

  // Emit signal that a Task was added
  emit TaskAdded(t);
  emit TaskListChanged();
}

void TaskManager::CancelTask(Task *t)
{
  if (failed_tasks_.contains(t)) {
    failed_tasks_.removeOne(t);
    emit TaskRemoved(t);
    t->deleteLater();
  } else {
    t->Cancel();
  }
}

void TaskManager::TaskFinished()
{
  QFutureWatcher<bool>* watcher = static_cast<QFutureWatcher<bool>*>(sender());
  Task* t = tasks_.value(watcher);

  tasks_.remove(watcher);

  if (watcher->result()) {
    // Task completed successfully
    emit TaskRemoved(t);
    t->deleteLater();
  } else {
    // Task failed, keep it so the user can see the error message
    emit TaskFailed(t);
    failed_tasks_.append(t);
  }

  watcher->deleteLater();

  emit TaskListChanged();
}

OLIVE_NAMESPACE_EXIT

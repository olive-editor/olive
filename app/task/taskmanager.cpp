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

TaskManager* TaskManager::instance_ = nullptr;

TaskManager::TaskManager() :
  active_thread_count_(0)
{
  // Initialize threads to run tasks on
  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    QThread* t = new QThread(this);
    t->start(QThread::LowPriority);
    threads_.replace(i, {t, false});
  }
}

TaskManager::~TaskManager()
{
  // First send the signal to all tasks to start cancelling
  foreach (const TaskContainer& task_info, tasks_) {
    if (task_info.status == kWorking) {
      task_info.task->Cancel();
    }
  }

  // Next, signal each thread to quit as its next event in the queue
  foreach (const ThreadContainer& tc, threads_) {
    tc.thread->quit();
  }

  // Wait for each thread's event queue to finish
  foreach (const ThreadContainer& tc, threads_) {
    tc.thread->wait();

    // This is technically unnecessary since each QThread is a child of this object, but we may as well
    delete tc.thread;
  }

  // Finally delete all task objects (they shouldn't have been deleted by TaskSucceeded() or TaskFailed() because our
  // event queue shouldn't be active by this point
  foreach (const TaskContainer& task_info, tasks_) {
    delete task_info.task;
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

void TaskManager::AddTask(Task* t)
{
  // Connect Task's status signal to the Callback
  connect(t, &Task::Succeeeded, this, &TaskManager::TaskSucceeded, Qt::QueuedConnection);
  connect(t, &Task::Failed, this, &TaskManager::TaskFailed, Qt::QueuedConnection);

  // Add the Task to the queue
  tasks_.append({t, kWaiting});

  // Emit signal that a Task was added
  emit TaskAdded(t);

  // Scan through queue and start any Tasks that can (including this one)
  StartNextWaiting();
}

void TaskManager::StartNextWaiting()
{
  // If there are no tasks in the queue, there is nothing to be done
  if (tasks_.isEmpty()) {
    return;
  }

  // If all threads are occupied, nothing to be done
  if (active_thread_count_ == threads_.size()) {
    return;
  }

  // Create a list of tasks that are waiting
  QList<Task*> waiting_tasks;
  foreach (const TaskContainer& task_info, tasks_) {
    if (task_info.status == kWaiting) {
      waiting_tasks.append(task_info.task);
    }
  }

  // No tasks waiting to start
  if (waiting_tasks.isEmpty()) {
    return;
  }

  // For any inactive threads,
  for (int i=0;i<threads_.size();i++) {
    if (!threads_.at(i).active) {
      // This thread is inactive and needs a new Task
      Task* task = waiting_tasks.takeFirst();

      task->moveToThread(threads_.at(i).thread);

      threads_[i].active = true;
      active_thread_count_++;

      SetTaskStatus(task, kWorking);

      QMetaObject::invokeMethod(task,
                                "Start",
                                Qt::QueuedConnection);

      if (active_thread_count_ == threads_.size() || waiting_tasks.isEmpty()) {
        break;
      }
    }
  }
}

void TaskManager::DeleteTask(Task *t)
{
  if (GetTaskStatus(t) == kWorking) {
    // Send a signal to the task to cancel, it will likely continue to cancel in the background after it's removed
    t->Cancel();
  }

  // Remove instances of Task from queue
  for (int i=0;i<tasks_.size();i++) {
    if (tasks_.at(i).task == t) {
      tasks_.removeAt(i);
      break;
    }
  }
  emit t->Removed();

  if (GetTaskStatus(t) != kWorking) {
    // If the task isn't doing anything, we can simply delete it
    delete t;
  }
}

void TaskManager::TaskFinished(Task* task)
{
  // Set this thread's active value to false
  for (int i=0;i<threads_.size();i++) {
    if (threads_.at(i).thread == task->thread()) {
      threads_[i].active = false;
    }
  }

  // Decrement the active thread count
  active_thread_count_--;
}

TaskManager::TaskStatus TaskManager::GetTaskStatus(Task *t)
{
  foreach (const TaskContainer& container, tasks_) {
    if (container.task == t) {
      return container.status;
    }
  }

  return kError;
}

void TaskManager::SetTaskStatus(Task *t, TaskStatus status)
{
  for (int i=0;i<tasks_.size();i++) {
    TaskContainer& cont = tasks_[i];

    if (cont.task == t) {
      cont.status = status;
      break;
    }
  }
}

void TaskManager::TaskSucceeded()
{
  Task* task_sender = static_cast<Task*>(sender());

  SetTaskStatus(task_sender, kFinished);

  TaskFinished(task_sender);

  // Delete this task
  DeleteTask(task_sender);
}

void TaskManager::TaskFailed()
{
  Task* task_sender = static_cast<Task*>(sender());

  SetTaskStatus(task_sender, kError);

  TaskFinished(task_sender);

  // If this task has already been deleted, we'll free its memory now
  bool was_deleted = true;

  for (int i=0;i<tasks_.size();i++) {
    if (tasks_.at(i).task == task_sender) {
      was_deleted = false;
      break;
    }
  }

  if (was_deleted) {
    delete task_sender;
  }
}

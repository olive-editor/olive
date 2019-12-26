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

TaskManager TaskManager::instance_;

TaskManager::TaskManager()
{
  maximum_task_count_ = QThread::idealThreadCount();
}

TaskManager::~TaskManager()
{
  Clear();
}

TaskManager *TaskManager::instance()
{
  return &instance_;
}

void TaskManager::AddTask(TaskPtr t)
{
  // Connect Task's status signal to the Callback
  connect(t.get(), SIGNAL(StatusChanged(Task::Status)), this, SLOT(TaskCallback(Task::Status)));

  // Add the Task to the queue
  tasks_.append(t);

  // Emit signal that a Task was added
  emit TaskAdded(t.get());

  // Scan through queue and start any Tasks that can (including this one)
  StartNextWaiting();
}

void TaskManager::Clear()
{
  // Delete Tasks from memory
  for (int i=0;i<tasks_.size();i++) {
    tasks_.at(i)->Cancel();
  }
  tasks_.clear();
}

void TaskManager::StartNextWaiting()
{
  // Count the tasks that are currently active
  int working_count = 0;

  for (int i=0;i<tasks_.size();i++) {
    TaskPtr t = tasks_.at(i);

    if (t->status() == Task::kWorking) {

      // Task is active, add it to the count
      working_count++;

    } else if (t->status() == Task::kWaiting) {

      // Task is waiting and we have available threads, try to start it
      if (t->Start()) {
        // If it started, add it to the working count
        working_count++;
      }

    }

    // Check if the count exceeds our maximum threads, if so stop here
    if (working_count == maximum_task_count_) {
      break;
    }
  }
}

void TaskManager::DeleteTask(Task *t)
{
  // Cancel the task
  t->Cancel();

  // Remove instances of Task from queue
  for (int i=0;i<tasks_.size();i++) {
    if (tasks_.at(i).get() == t) {
      emit t->Removed();
      tasks_.removeAt(i);
      break;
    }
  }
}

void TaskManager::TaskCallback(Task::Status status)
{
  if (status == Task::kFinished || status == Task::kError) {
    // The Task has finished, we can start a new one
    StartNextWaiting();

    if (status == Task::kFinished) {
      // The Task was successful, remove this Task from the queue
      DeleteTask(static_cast<Task*>(sender()));
    }
  }
}

TaskManager::AddTaskCommand::AddTaskCommand(TaskPtr t, QUndoCommand *parent) :
  QUndoCommand(parent),
  task_(t)
{
}

void TaskManager::AddTaskCommand::redo()
{
  TaskManager::instance()->AddTask(task_);
}

void TaskManager::AddTaskCommand::undo()
{
  TaskManager::instance()->DeleteTask(task_.get());

  task_->ResetState();
}

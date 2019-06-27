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

TaskManager olive::task_manager;

TaskManager::TaskManager()
{
  maximum_task_count_ = QThread::idealThreadCount();
}

void TaskManager::AddTask(Task* t)
{
  // Connect Task's status signal to the Callback
  connect(t, SIGNAL(StatusChanged(Task::Status)), this, SLOT(TaskCallback(Task::Status)));

  // Add the Task to the queue
  tasks_.append(t);

  // Scan through queue and start any Tasks that can (including this one)
  StartNextWaiting();
}

void TaskManager::StartNextWaiting()
{
  // Count the tasks that are currently active
  int working_count = 0;

  for (int i=0;i<tasks_.size();i++) {
    Task* t = tasks_.at(i);

    if (t->status() == Task::kWorking) {
      // Task is active, add it to the count
      working_count++;
    } else if (t->status() == Task::kWaiting) {
      // Task is waiting and we have available threads, start it and add it to the count
      t->Start();
      working_count++;
    }

    // Check if the count exceeds our maximum threads, if so stop here
    if (working_count == maximum_task_count_) {
      break;
    }
  }
}

void TaskManager::TaskCallback(Task::Status status)
{
  switch (status) {
  case Task::kWaiting:
    qDebug() << sender() << "is waiting...";
    break;
  case Task::kWorking:
    qDebug() << sender() << "is working...";
    break;
  case Task::kFinished:
    qDebug() << sender() << "finished successfully.";
    StartNextWaiting();
    break;
  case Task::kError:
    qDebug() << sender() << "failed:" << static_cast<Task*>(sender())->error();
    StartNextWaiting();
    break;
  }
}

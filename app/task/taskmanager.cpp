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
  connect(t, SIGNAL(StatusChanged(Task::Status)), this, SLOT(TaskCallback(Task::Status)));

  tasks_.append(t);

  if (tasks_.size() < maximum_task_count_) {
    t->Start();
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
    break;
  case Task::kError:
    qDebug() << sender() << "failed:" << static_cast<Task*>(sender())->error();
    break;
  }
}

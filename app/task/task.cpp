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

#include "task.h"

Task::Task() :
  status_(kWaiting),
  thread_(this)
{
  connect(&thread_, SIGNAL(finished()), this, SLOT(ThreadComplete()));
}

void Task::Start()
{
  set_status(kWorking);

  thread_.start();
}

bool Task::Action()
{
  return true;
}

const Task::Status &Task::status()
{
  return status_;
}

const QString &Task::error()
{
  return error_;
}

void Task::SetError(const QString &s)
{
  error_ = s;
}

void Task::set_status(const Task::Status &status)
{
  status_ = status;

  emit StatusChanged(status_);
}

void Task::ThreadComplete()
{
  if (thread_.result()) {
    set_status(kFinished);
  } else {
    set_status(kError);
  }
}

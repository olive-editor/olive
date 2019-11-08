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
  thread_(this),
  text_(tr("Task")),
  cancelled_(false)
{
  connect(&thread_, SIGNAL(finished()), this, SLOT(ThreadComplete()));
}

bool Task::Start()
{
  // Tasks can only start if they're waiting
  if (status_ != kWaiting) {
    return false;
  }

  // Check if this task has any dependencies (tasks that should complete before this one starts)
  for (Task *dependency : dependencies_) {
    if (dependency->status() == kWaiting || dependency->status() == kWorking) {

      // We need this task to finish before this task can start, so keep waiting
      return false;

    } 
    if (dependency->status() == kError) {

      // A dependency errored, so this task is likely invalid too
      set_error(tr("A dependency task failed"));
      set_status(kError);
      return false;

    }
  }

  cancelled_ = false;

  // Run Prologue() function
  if (!Prologue()) {
    set_status(kError);
    return false;
  }

  set_status(kWorking);

  thread_.start();

  return true;
}

bool Task::Prologue()
{
  return true;
}

bool Task::Action()
{
  return true;
}

bool Task::Epilogue()
{
  return true;
}

const Task::Status &Task::status()
{
  return status_;
}

const QString &Task::text()
{
  return text_;
}

const QString &Task::error()
{
  return error_;
}

void Task::AddDependency(Task *dependency)
{
  // Dependencies cannot be added if the Task is working or complete
  Q_ASSERT(status_ == kWaiting);

  dependencies_.append(dependency);
}

void Task::ResetState()
{
  if (status_ == kWaiting) {
    return;
  }

  if (status_ == kWorking) {
    Cancel();
  }

  cancelled_ = false;

  set_error(QString());

  set_status(kWaiting);
}

void Task::Cancel()
{
  if (status_ == kWaiting) {
    set_status(kFinished);
  }

  if (status_ != kWorking) {
    return;
  }

  cancelled_ = true;

  // FIXME: Should we limit the wait time?
  thread_.wait();
}

void Task::set_error(const QString &s)
{
  error_ = s;
}

void Task::set_text(const QString &s)
{
  text_ = s;
}

bool Task::cancelled()
{
  return cancelled_;
}

void Task::set_status(const Task::Status &status)
{
  status_ = status;

  if (status_ == kFinished || status_ == kError) {
    emit Finished();
  }

  emit StatusChanged(status_);
}

void Task::ThreadComplete()
{
  // thread_.result() will be set to the return value of Action()
  bool succeeded = thread_.result();

  // Run the Epilogue() function for any final tasks
  // User cancelling is not considered an error, so we need to check it too
  if (succeeded && !cancelled()) {
    succeeded = Epilogue();
  }

  // If everything succeeded, we set the status accordingly
  if (succeeded) {
    set_status(kFinished);
  } else {
    set_status(kError);
  }
}

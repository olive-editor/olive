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

#include <QtConcurrent/QtConcurrent>

OLIVE_NAMESPACE_ENTER

TaskDialog::TaskDialog(Task* task, const QString& title, QWidget *parent) :
  ProgressDialog(task->GetTitle(), title, parent),
  task_(task),
  destroy_on_close_(true)
{
  // Clear task when this dialog is destroyed
  task_->setParent(this);

  // Connect the save manager progress signal to the progress bar update on the dialog
  connect(task_, &Task::ProgressChanged, this, &TaskDialog::SetProgress, Qt::QueuedConnection);

  // Connect cancel signal (must be a direct connection or it'll be queued after the task has
  // already finished)
  connect(this, &TaskDialog::Cancelled, task_, &Task::Cancel, Qt::DirectConnection);
}

void TaskDialog::showEvent(QShowEvent *e)
{
  ProgressDialog::showEvent(e);

  // Create watcher for when the task finishes
  QFutureWatcher<bool>* task_watcher = new QFutureWatcher<bool>();

  // Listen for when the task finishes
  connect(task_watcher, &QFutureWatcher<bool>::finished,
          this, &TaskDialog::TaskFinished, Qt::QueuedConnection);

  // Run task in another thread with QtConcurrent
  task_watcher->setFuture(QtConcurrent::run(task_, &Task::Run));
}

void TaskDialog::closeEvent(QCloseEvent *e)
{
  // Cancel task if it is running
  task_->Cancel();

  // Standard close function
  ProgressDialog::closeEvent(e);

  // Clean up this task and dialog
  if (destroy_on_close_) {
    deleteLater();
  }
}

void TaskDialog::TaskFinished()
{
  QFutureWatcher<bool>* task_watcher = static_cast<QFutureWatcher<bool>*>(sender());

  if (task_watcher->result()) {
    emit TaskSucceeded(task_);
  } else {
    ShowErrorMessage(tr("Task Failed"), task_->GetError());
    emit TaskFailed(task_);
  }

  task_watcher->deleteLater();

  close();
}

OLIVE_NAMESPACE_EXIT

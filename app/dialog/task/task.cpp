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

#include <QMessageBox>
#include <QThread>

OLIVE_NAMESPACE_ENTER

TaskDialog::TaskDialog(Task* task, const QString& title, QWidget *parent) :
  ProgressDialog(task->GetTitle(), title, parent),
  task_(task),
  task_failed_(false)
{
  thread_ = new QThread();

  // Connect the save manager progress signal to the progress bar update on the dialog
  connect(task_, &Task::ProgressChanged, this, &TaskDialog::SetProgress, Qt::QueuedConnection);

  // Connect error reporting
  connect(task_, &Task::Failed, this, &TaskDialog::TaskFailed, Qt::QueuedConnection);

  // Connect cancel signal (must be a direct connection or it'll be queued after the task has already finished)
  connect(this, &TaskDialog::Cancelled, task_, &Task::Cancel, Qt::DirectConnection);

  // Connect cleanup functions (ensure everything new'd in this function is deleteLater'd)
  connect(task_, &Task::Finished, this, &TaskDialog::close, Qt::QueuedConnection);

  // When task is finished, signal thread to quit
  connect(task_, &Task::Finished, thread_, &QThread::quit, Qt::QueuedConnection);

  // When thread has quit, delete both task and thread
  connect(thread_, &QThread::finished, task_, &Task::deleteLater, Qt::QueuedConnection);
  connect(thread_, &QThread::finished, thread_, &QThread::deleteLater, Qt::QueuedConnection);
}

void TaskDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  // Create a separate thread to run this task in
  thread_->start();

  // Move the task to this thread
  task_->moveToThread(thread_);

  // Start the task
  QMetaObject::invokeMethod(task_, "Start", Qt::QueuedConnection);
}

void TaskDialog::closeEvent(QCloseEvent *e)
{
  // Show error if the task failed
  if (!task_->IsCancelled() && task_failed_) {
    QMessageBox::critical(this,
                          tr("Error"),
                          task_error_,
                          QMessageBox::Ok);
  }

  // Cancel task if it is running
  task_->Cancel();

  // Standard close function
  QDialog::closeEvent(e);

  // Clean up this dialog (FIXME: Is this necessary?)
  deleteLater();
}

void TaskDialog::TaskFailed(const QString &s)
{
  task_failed_ = true;
  task_error_ = s;
}

OLIVE_NAMESPACE_EXIT

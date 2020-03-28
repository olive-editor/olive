#include "task.h"

#include <QMessageBox>
#include <QThread>

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

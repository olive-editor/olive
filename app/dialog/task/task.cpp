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

  // Connect cancel signal (must be a direct connection or it'll be queued after the save is already finished)
  connect(this, &TaskDialog::Cancelled, this, &TaskDialog::reject, Qt::DirectConnection);

  // Connect cleanup functions (ensure everything new'd in this function is deleteLater'd)
  connect(task_, &Task::Finished, this, &TaskDialog::accept, Qt::QueuedConnection);
  connect(task_, &Task::Finished, this, &TaskDialog::deleteLater, Qt::QueuedConnection);
  connect(task_, &Task::Finished, task_, &Task::deleteLater, Qt::QueuedConnection);
  connect(task_, &Task::Finished, thread_, &QThread::quit, Qt::QueuedConnection);
  connect(thread_, &QThread::finished, thread_, &QThread::deleteLater, Qt::QueuedConnection);
}

void TaskDialog::open()
{
  ProgressDialog::open();

  if (thread_->isRunning()) {
    // Task has already started, no need to do anymore
    return;
  }

  // Create a separate thread to run this task in
  thread_->start();

  // Move the task to this thread
  task_->moveToThread(thread_);

  // Start the task
  QMetaObject::invokeMethod(task_, "Start", Qt::QueuedConnection);
}

void TaskDialog::accept()
{
  if (task_failed_) {
    QMessageBox::critical(this,
                          tr("Task Error"),
                          task_error_,
                          QMessageBox::Ok);
  }

  ProgressDialog::accept();
}

void TaskDialog::reject()
{
  task_->Cancel();
}

void TaskDialog::TaskFailed(const QString &s)
{
  task_failed_ = true;
  task_error_ = s;
}

#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include "dialog/progress/progress.h"
#include "task/task.h"

class TaskDialog : public ProgressDialog
{
public:
  TaskDialog(Task *task, const QString &title, QWidget* parent = nullptr);

public slots:
  virtual void open() override;

  virtual void accept() override;
  virtual void reject() override;

private:
  Task* task_;

  QThread* thread_;

  bool task_failed_;
  QString task_error_;

private slots:
  void TaskFailed(const QString& s);

};

#endif // TASKDIALOG_H

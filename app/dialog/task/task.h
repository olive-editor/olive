/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include "dialog/progress/progress.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class TaskDialog : public ProgressDialog
{
  Q_OBJECT
public:
  /**
   * @brief TaskDialog Constructor
   *
   * Creates a TaskDialog. The TaskDialog takes ownership of the Task and will destroy it on close.
   * Connect to the Task::Succeeded() if you want to retrieve information from the task before it
   * gets destroyed.
   */
  TaskDialog(Task *task, const QString &title, QWidget* parent = nullptr);

  /**
   * @brief Set whether TaskDialog should destroy itself (and the task) when it's closed
   *
   * This is TRUE by default.
   */
  void SetDestroyOnClose(bool e)
  {
    destroy_on_close_ = e;
  }

  /**
   * @brief Returns this dialog's task
   */
  Task* GetTask() const
  {
    return task_;
  }

protected:
  virtual void showEvent(QShowEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

signals:
  void TaskSucceeded(Task* task);

  void TaskFailed(Task* task);

private:
  Task* task_;

  bool destroy_on_close_;

private slots:
  void TaskFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // TASKDIALOG_H

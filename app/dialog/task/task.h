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

#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include "dialog/progress/progress.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class TaskDialog : public ProgressDialog
{
  Q_OBJECT
public:
  TaskDialog(Task *task, const QString &title, QWidget* parent = nullptr);

protected:
  virtual void showEvent(QShowEvent* e) override;

  virtual void closeEvent(QCloseEvent* e) override;

private:
  Task* task_;

  QThread* thread_;

  bool task_failed_;
  QString task_error_;

private slots:
  void TaskFailed(const QString& s);

};

OLIVE_NAMESPACE_EXIT

#endif // TASKDIALOG_H

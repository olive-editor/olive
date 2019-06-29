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

#ifndef TASKVIEWITEM_H
#define TASKVIEWITEM_H

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

#include "task/task.h"

class TaskViewItem : public QFrame
{
  Q_OBJECT
public:
  TaskViewItem(QWidget* parent);

  void SetTask(Task* t);

private:
  QLabel* task_name_lbl_;
  QProgressBar* progress_bar_;
  QPushButton* cancel_btn_;
  QLabel* task_status_lbl_;

  Task* task_;

private slots:
  void TaskStatusChange(Task::Status status);

};

#endif // TASKVIEWITEM_H

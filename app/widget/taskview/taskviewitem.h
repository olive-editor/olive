/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QStackedWidget>
#include <QWidget>

#include "elapsedcounterwidget.h"
#include "task/task.h"

namespace olive {

/**
 * @brief A widget that visually represents the status of a Task
 *
 * The TaskViewItem widget shows a description of the Task (Task::text(), a progress bar (updated by
 * Task::ProgressChanged), the Task's status (text generated from Task::status() or Task::error()), and provides
 * a cancel button (triggering Task::Cancel()) for cancelling a Task before it finishes.
 *
 * The main entry point is SetTask() after a Task and TaskViewItem objects are created.
 */
class TaskViewItem : public QFrame
{
  Q_OBJECT
public:
  TaskViewItem(Task *task, QWidget* parent = nullptr);

  void Failed();

signals:
  void TaskCancelled(Task* t);

private:
  QLabel* task_name_lbl_;
  QProgressBar* progress_bar_;
  QPushButton* cancel_btn_;

  QStackedWidget* status_stack_;
  ElapsedCounterWidget* elapsed_timer_lbl_;
  QLabel* task_error_lbl_;

  Task* task_;

private slots:
  void UpdateProgress(double d);

};

}

#endif // TASKVIEWITEM_H

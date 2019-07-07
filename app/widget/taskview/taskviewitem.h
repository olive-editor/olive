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
  TaskViewItem(QWidget* parent);

  /**
   * @brief Connects a Task to this object
   *
   * If a Task has already been connected, this will disconnect this TaskViewItem from the previously connected
   * Task before connecting to the next one - however there are very few circumstances where this would be necessary
   * since TaskViewItem is designed to delete itself when a Task is complete.
   */
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

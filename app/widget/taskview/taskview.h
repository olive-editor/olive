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

#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QScrollArea>
#include <QVBoxLayout>

#include "widget/taskview/taskviewitem.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A widget that shows a list of Tasks
 *
 * TaskView is a fairly simple widget for showing TaskViewItem widgets that each represent a Task object. The main
 * entry point is the slot AddTask() which should be connected to a TaskManager's TaskAdded() signal. No more connecting
 * is necessary since TaskViewItem will automatically delete itself (thus removing itself from the TaskView) when the
 * Task finishes.
 */
class TaskView : public QScrollArea
{
  Q_OBJECT
public:
  TaskView(QWidget* parent);

public slots:
  /**
   * @brief Creates a TaskViewItem, connects it to a Task, and adds it to this widget
   *
   * Connect this to TaskManager::TaskAdded().
   */
  void AddTask(Task* t);

  void TaskFailed(Task* t);

  void RemoveTask(Task* t);

private:
  QWidget* central_widget_;

  QVBoxLayout* layout_;

  QHash<Task*, TaskViewItem*> items_;

};

OLIVE_NAMESPACE_EXIT

#endif // TASKVIEW_H

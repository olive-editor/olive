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

#include "taskmanager.h"

#include "task/taskmanager.h"

OLIVE_NAMESPACE_ENTER

TaskManagerPanel::TaskManagerPanel(QWidget* parent) :
  PanelWidget(QStringLiteral("TaskManagerPanel"), parent)
{
  // Create task view
  view_ = new TaskView(this);

  // Set it as the main widget
  setWidget(view_);

  // Connect task view to the task manager
  connect(TaskManager::instance(), &TaskManager::TaskAdded, view_, &TaskView::AddTask);
  connect(TaskManager::instance(), &TaskManager::TaskRemoved, view_, &TaskView::RemoveTask);
  connect(TaskManager::instance(), &TaskManager::TaskFailed, view_, &TaskView::TaskFailed);
  connect(view_, &TaskView::TaskCancelled, TaskManager::instance(), &TaskManager::CancelTask);

  // Set strings
  Retranslate();
}

void TaskManagerPanel::Retranslate()
{
  SetTitle(tr("Task Manager"));
}

OLIVE_NAMESPACE_EXIT

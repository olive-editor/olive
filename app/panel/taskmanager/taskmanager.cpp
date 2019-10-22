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

TaskManagerPanel::TaskManagerPanel(QWidget* parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("TaskManagerPanel");

  // Create task view
  view_ = new TaskView(this);

  // Set it as the main widget
  setWidget(view_);

  // Connect task view to the task manager
  connect(&olive::task_manager, SIGNAL(TaskAdded(Task*)), view_, SLOT(AddTask(Task*)));

  // Set strings
  Retranslate();
}

void TaskManagerPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  PanelWidget::changeEvent(e);
}

void TaskManagerPanel::Retranslate()
{
  SetTitle(tr("Task Manager"));
}

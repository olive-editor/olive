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

#include "taskview.h"

#include <QPushButton>

OLIVE_NAMESPACE_ENTER

TaskView::TaskView(QWidget* parent) :
  QScrollArea(parent)
{
  // Allow scroll area to resize widget to fit
  setWidgetResizable(true);

  // Create central widget
  central_widget_ = new QWidget(this);
  setWidget(central_widget_);

  // Create layout for central widget
  layout_ = new QVBoxLayout(central_widget_);
  layout_->setSpacing(0);
  layout_->setMargin(0);

  // Add a "stretch" so that TaskViewItems don't try to expand all the way to the bottom
  layout_->addStretch();
}

void TaskView::AddTask(Task *t)
{
  // Create TaskViewItem (UI representation of a Task) and connect it
  TaskViewItem* item = new TaskViewItem(t);
  connect(item, &TaskViewItem::TaskCancelled, this, &TaskView::TaskCancelled);
  items_.insert(t, item);
  layout_->insertWidget(layout_->count()-1, item);
}

void TaskView::TaskFailed(Task *t)
{
  items_.value(t)->Failed();
}

void TaskView::RemoveTask(Task *t)
{
  items_.value(t)->deleteLater();
  items_.remove(t);
}

OLIVE_NAMESPACE_EXIT

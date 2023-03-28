/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "historywidget.h"

#include "core.h"

namespace olive {

HistoryWidget::HistoryWidget(QWidget *parent) :
  QTreeView(parent)
{
  stack_ = Core::instance()->undo_stack();

  this->setModel(stack_);
  this->setRootIsDecorated(false);
  connect(stack_, &UndoStack::indexChanged, this, &HistoryWidget::indexChanged);
  connect(this->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &HistoryWidget::currentRowChanged);
}

void HistoryWidget::indexChanged(int i)
{
  this->selectionModel()->select(this->model()->index(i-1, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void HistoryWidget::currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
  size_t jump_to = (current.row() + 1);
  stack_->jump(jump_to);
}

}

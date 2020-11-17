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

#include "nodecombobox.h"

#include <QAction>
#include <QDebug>

#include "node/factory.h"
#include "ui/icons/icons.h"
#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

NodeComboBox::NodeComboBox(QWidget *parent) :
  QComboBox(parent)
{
}

void NodeComboBox::showPopup()
{
  Menu* m = NodeFactory::CreateMenu(this, true);

  QAction* selected = m->exec(parentWidget()->mapToGlobal(pos()));

  if (selected) {
    QString new_id = NodeFactory::GetIDFromMenuAction(selected);

    SetNodeInternal(new_id, true);
  }

  delete m;
}

const QString &NodeComboBox::GetSelectedNode() const
{
  return selected_id_;
}

void NodeComboBox::SetNode(const QString &id)
{
  SetNodeInternal(id, false);
}

void NodeComboBox::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    UpdateText();
  }

  QComboBox::changeEvent(e);
}

void NodeComboBox::UpdateText()
{
  clear();

  if (!selected_id_.isEmpty()) {
    addItem(NodeFactory::GetNameFromID(selected_id_));
  }
}

void NodeComboBox::SetNodeInternal(const QString &id, bool emit_signal)
{
  if (selected_id_ != id) {
    selected_id_ = id;

    UpdateText();

    if (emit_signal) {
      emit NodeChanged(selected_id_);
    }
  }
}

OLIVE_NAMESPACE_EXIT

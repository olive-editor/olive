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

#include "item.h"

#include "folder/folder.h"

namespace olive {

#define super QObject

const QString Item::kParentInput = QStringLiteral("parent_in");

Item::Item(bool create_folder_input, bool create_default_output) :
  Node(create_default_output)
{
  if (create_folder_input) {
    // Hierarchy input for items
    AddInput(kParentInput, NodeValue::kNone);
    IgnoreHashingFrom(kParentInput);
    IgnoreInvalidationsFrom(kParentInput);
  }
}

const QString &Item::tooltip() const
{
  return tooltip_;
}

void Item::set_tooltip(const QString &t)
{
  tooltip_ = t;
}

QString Item::duration()
{
  return QString();
}

QString Item::rate()
{
  return QString();
}

Folder *Item::item_parent() const
{
  return dynamic_cast<Folder*>(GetConnectedNode(kParentInput));
}

void Item::Retranslate()
{
  SetInputName(kParentInput, tr("Folder"));
}

}

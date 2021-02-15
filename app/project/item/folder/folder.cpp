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

#include "folder.h"

#include "common/xmlutils.h"
#include "project/item/footage/footage.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"

namespace olive {

Folder::Folder()
{
}

QIcon Folder::icon() const
{
  return icon::Folder;
}

bool ChildExistsWithNameInternal(const Folder* n, const QString& s)
{
  foreach (const Node::OutputConnection& c, n->output_connections()) {
    Node* connected = c.second.node();

    if (connected->GetLabel() == s) {
      return true;
    } else {
      Folder* subfolder = dynamic_cast<Folder*>(connected);

      if (subfolder && ChildExistsWithNameInternal(subfolder, s)) {
        return true;
      }
    }
  }

  return false;
}

bool Folder::ChildExistsWithName(const QString &s) const
{
  return ChildExistsWithNameInternal(this, s);
}

void Folder::OutputConnectedEvent(const QString &output, const NodeInput &input)
{
  Q_UNUSED(output)

  Item* item = dynamic_cast<Item*>(input.node());

  if (item) {
    // The insert index is always our "count" because we only support appending in our internal
    // model. For sorting/organizing, a QSortFilterProxyModel is used instead.
    emit BeginInsertItem(item, item_child_count());
    item_children_.append(item);
    emit EndInsertItem();
  }
}

void Folder::OutputDisconnectedEvent(const QString &output, const NodeInput &input)
{
  Q_UNUSED(output)

  Item* item = dynamic_cast<Item*>(input.node());

  if (item) {
    int child_index = item_children_.indexOf(item);
    emit BeginRemoveItem(item, child_index);
    item_children_.removeAt(child_index);
    emit EndRemoveItem();
  }
}

}

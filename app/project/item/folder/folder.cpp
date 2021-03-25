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

#define super Node

const QString Folder::kChildInput = QStringLiteral("child_in");

Folder::Folder()
{
  AddInput(kChildInput, NodeValue::kNone, InputFlags(kInputFlagArray | kInputFlagNotKeyframable));
}

QIcon Folder::icon() const
{
  return icon::Folder;
}

void Folder::Retranslate()
{
  super::Retranslate();

  SetInputName(kChildInput, tr("Children"));
}

bool ChildExistsWithNameInternal(const Folder* n, const QString& s)
{
  for (int i=0; i<n->item_child_count(); i++) {
    Node* child = n->item_child(i);

    if (child->GetLabel() == s) {
      return true;
    } else {
      Folder* subfolder = dynamic_cast<Folder*>(child);

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

int Folder::index_of_child_in_array(Node *item) const
{
  int index_of_item = item_children_.indexOf(item);

  if (index_of_item == -1) {
    return -1;
  }

  return item_element_index_.at(index_of_item);
}

void Folder::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kChildInput && element != -1) {
    Node* item = output.node();

    // The insert index is always our "count" because we only support appending in our internal
    // model. For sorting/organizing, a QSortFilterProxyModel is used instead.
    emit BeginInsertItem(item, item_child_count());
    item_children_.append(item);
    item_element_index_.append(element);
    item->SetFolder(this);
    emit EndInsertItem();
  }
}

void Folder::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  if (input == kChildInput && element != -1) {
    Node* item = output.node();

    int child_index = item_children_.indexOf(item);
    emit BeginRemoveItem(item, child_index);
    item_children_.removeAt(child_index);
    item_element_index_.removeAt(child_index);
    item->SetFolder(nullptr);
    emit EndRemoveItem();
  }
}

FolderAddChild::FolderAddChild(Folder *folder, Node *child, bool autoposition) :
  folder_(folder),
  child_(child),
  autoposition_(autoposition),
  position_command_(nullptr)
{
}

FolderAddChild::~FolderAddChild()
{
  delete position_command_;
}

Project *FolderAddChild::GetRelevantProject() const
{
  return folder_->project();
}

void FolderAddChild::redo()
{
  int array_index = folder_->InputArraySize(Folder::kChildInput);
  folder_->InputArrayAppend(Folder::kChildInput, false);
  Node::ConnectEdge(child_, NodeInput(folder_, Folder::kChildInput, array_index));

  if (autoposition_) {
    old_position_ = child_->GetPosition();
    if (!position_command_) {
      position_command_ = new NodeSetPositionAsChildCommand(child_, folder_, array_index, array_index+1, true);
    }
    position_command_->redo();
  }
}

void FolderAddChild::undo()
{
  if (position_command_) {
    position_command_->undo();
  }

  Node::DisconnectEdge(child_, NodeInput(folder_, Folder::kChildInput, folder_->InputArraySize(Folder::kChildInput)-1));
  folder_->InputArrayRemoveLast(Folder::kChildInput);
}

}

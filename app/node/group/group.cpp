/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "group.h"

#include "node/graph.h"

namespace olive {

NodeGroup::NodeGroup() :
  output_passthrough_(nullptr)
{
  graph_ = new NodeGraph();
  graph_->setParent(this);
}

QString NodeGroup::Name() const
{
  if (custom_name_.isEmpty()) {
    return tr("Group");
  } else {
    return custom_name_;
  }
}

QString NodeGroup::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.group");
}

QVector<Node::CategoryID> NodeGroup::Category() const
{
  return {kCategoryGeneral};
}

QString NodeGroup::Description() const
{
  return tr("A group of nodes that is represented as a single node.");
}

void NodeGroup::Retranslate()
{
  foreach (Node *n, graph_->nodes()) {
    n->Retranslate();
  }
}

void NodeGroup::AddNode(Node *node)
{
  node->setParent(graph_);
}

void NodeGroup::RemoveNode(Node *node, QObject *new_parent)
{
  if (node->parent() == graph_) {
    node->setParent(new_parent);
  }
}

void NodeGroup::AddInputPassthrough(const NodeInput &input)
{
  Q_ASSERT(graph_->nodes().contains(input.node()));

  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it.value() == input) {
      // Already passing this input through
      return;
    }
  }

  // Add input
  QString id = GetGroupInputIDFromInput(input);

  AddInput(id, input.GetDataType(), input.GetDefaultValue(), input.GetFlags());

  input_passthroughs_.insert(id, input);
}

void NodeGroup::RemoveInputPassthrough(const NodeInput &input)
{
  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it.value() == input) {
      RemoveInput(it.key());
      input_passthroughs_.erase(it);
      break;
    }
  }
}

void NodeGroup::SetOutputPassthrough(Node *node)
{
  Q_ASSERT(graph_->nodes().contains(node));

  output_passthrough_ = node;
}

QString NodeGroup::GetGroupInputIDFromInput(const NodeInput &input)
{
  QCryptographicHash hash(QCryptographicHash::Sha1);

  hash.addData(input.node()->GetUUID().toByteArray());

  hash.addData(input.input().toUtf8());

  hash.addData((const char*) &input.element(), sizeof(input.element()));

  return QString::fromLatin1(hash.result().toHex());
}

bool NodeGroup::ContainsInputPassthrough(const NodeInput &input) const
{
  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it.value() == input) {
      return true;
    }
  }

  return false;
}

void NodeAddToGroupCommand::redo()
{
  previous_parent_ = node_->parent();
  group_->AddNode(node_);
}

void NodeAddToGroupCommand::undo()
{
  group_->RemoveNode(node_, previous_parent_);
}

void NodeGroupSetCustomNameCommand::redo()
{
  old_name_ = group_->GetCustomName();
  group_->SetCustomName(new_name_);
}

void NodeGroupSetCustomNameCommand::undo()
{
  group_->SetCustomName(old_name_);
}

void NodeGroupAddInputPassthrough::redo()
{
  if (!group_->ContainsInputPassthrough(input_)) {
    group_->AddInputPassthrough(input_);
    actually_added_ = true;
  } else {
    actually_added_ = false;
  }
}

void NodeGroupAddInputPassthrough::undo()
{
  if (actually_added_) {
    group_->RemoveInputPassthrough(input_);
  }
}

}

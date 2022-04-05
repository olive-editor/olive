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

#define super Node

NodeGroup::NodeGroup() :
  output_passthrough_(nullptr)
{
}

QString NodeGroup::Name() const
{
  return tr("Group");
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
  for (auto it=GetContextPositions().cbegin(); it!=GetContextPositions().cend(); it++) {
    it.key()->Retranslate();
  }
}

QString NodeGroup::AddInputPassthrough(const NodeInput &input, const QString &force_id)
{
  Q_ASSERT(ContextContainsNode(input.node()));

  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it->second == input) {
      // Already passing this input through
      return it->first;
    }
  }

  // Add input
  QString id;
  if (force_id.isEmpty()) {
    id = input.input();
    int i = 2;
    while (HasInputWithID(id)) {
      id = QStringLiteral("%1_%2").arg(input.name(), QString::number(i));
      i++;
    }
  } else {
    id = force_id;

    bool already_exists = false;
    for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
      if (it->first == id) {
        already_exists = true;
        break;
      }
    }

    Q_ASSERT(!already_exists);
  }

  AddInput(id, input.GetDataType(), input.GetDefaultValue(), input.GetFlags());

  input_passthroughs_.append({id, input});

  emit InputPassthroughAdded(this, input);

  return id;
}

void NodeGroup::RemoveInputPassthrough(const NodeInput &input)
{
  for (auto it=input_passthroughs_.begin(); it!=input_passthroughs_.end(); it++) {
    if (it->second == input) {
      RemoveInput(it->first);
      emit InputPassthroughRemoved(this, it->second);
      input_passthroughs_.erase(it);
      break;
    }
  }
}

void NodeGroup::SetOutputPassthrough(Node *node)
{
  Q_ASSERT(!node || ContextContainsNode(node));

  output_passthrough_ = node;

  emit OutputPassthroughChanged(this, output_passthrough_);
}

bool NodeGroup::ContainsInputPassthrough(const NodeInput &input) const
{
  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it->second == input) {
      return true;
    }
  }

  return false;
}

QString NodeGroup::GetInputName(const QString &id) const
{
  // If an override name was set, use that
  QString override = super::GetInputName(id);
  if (!override.isEmpty()) {
    return override;
  }

  // Call GetInputName of passed through node, which may be another group
  NodeInput pass = GetInputFromID(id);
  return pass.node()->GetInputName(pass.input());
}

NodeInput NodeGroup::ResolveInput(NodeInput input)
{
  while (GetInner(&input)) {}

  return input;
}

bool NodeGroup::GetInner(NodeInput *input)
{
  if (NodeGroup *g = dynamic_cast<NodeGroup*>(input->node())) {
    const NodeInput &passthrough = g->GetInputFromID(input->input());
    input->set_node(passthrough.node());
    input->set_input(passthrough.input());
    return true;
  } else {
    return false;
  }
}

void NodeGroupAddInputPassthrough::redo()
{
  if (!group_->ContainsInputPassthrough(input_)) {
    group_->AddInputPassthrough(input_, force_id_);
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

void NodeGroupSetOutputPassthrough::redo()
{
  old_output_ = group_->GetOutputPassthrough();
  group_->SetOutputPassthrough(new_output_);
}

void NodeGroupSetOutputPassthrough::undo()
{
  group_->SetOutputPassthrough(old_output_);
}

}

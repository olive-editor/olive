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

#ifndef NODEGROUP_H
#define NODEGROUP_H

#include "node/node.h"

namespace olive {

class NodeGroup : public Node
{
  Q_OBJECT
public:
  NodeGroup();

  NODE_DEFAULT_DESTRUCTOR(NodeGroup)
  NODE_COPY_FUNCTION(NodeGroup)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  void AddNode(Node *node);

  void RemoveNode(Node *node);

  bool ContainsNode(Node *node) const
  {
    return nodes_.contains(node);
  }

  const QVector<Node*> &GetNodes() const
  {
    return nodes_;
  }

  void AddInputPassthrough(const NodeInput &input);

  void RemoveInputPassthrough(const NodeInput &input);

  Node *GetOutputPassthrough() const
  {
    return output_passthrough_;
  }

  void SetOutputPassthrough(Node *node);

  const QString &GetCustomName() const
  {
    return custom_name_;
  }

  void SetCustomName(const QString &name)
  {
    custom_name_ = name;

    // NOTE: Not technically the right signal, but should achieve the right goal
    emit LabelChanged(custom_name_);
  }

  void ClearCustomName()
  {
    custom_name_.clear();
  }

  static QString GetGroupInputIDFromInput(const NodeInput &input);

  const QHash<QString, NodeInput> &GetInputPassthroughs() const
  {
    return input_passthroughs_;
  }

  bool ContainsInputPassthrough(const NodeInput &input) const;

  virtual QString GetInputName(const QString& id) const override;

signals:
  void NodeAddedToGroup(Node *node);

  void NodeRemovedFromGroup(Node *node);

  void InputPassthroughAdded(NodeGroup *group, const NodeInput &input);

  void InputPassthroughRemoved(NodeGroup *group, const NodeInput &input);

  void OutputPassthroughChanged(NodeGroup *group, Node *output);

private:
  QVector<Node*> nodes_;

  QHash<QString, NodeInput> input_passthroughs_;

  Node *output_passthrough_;

  QString custom_name_;

};

class NodeAddToGroupCommand : public UndoCommand
{
public:
  NodeAddToGroupCommand(Node *node, NodeGroup *group) :
    node_(node),
    group_(group)
  {}

  virtual Project * GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node *node_;

  NodeGroup *group_;

};

class NodeRemoveFromGroupCommand : public UndoCommand
{
public:
  NodeRemoveFromGroupCommand(Node *node, NodeGroup *group) :
    node_(node),
    group_(group)
  {}

  virtual Project * GetRelevantProject() const override
  {
    return node_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node *node_;

  NodeGroup *group_;

};

class NodeGroupSetCustomNameCommand : public UndoCommand
{
public:
  NodeGroupSetCustomNameCommand(NodeGroup *group, const QString &name) :
    group_(group),
    new_name_(name)
  {}

  virtual Project * GetRelevantProject() const override
  {
    return group_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  NodeGroup *group_;

  QString old_name_;

  QString new_name_;

};

class NodeGroupAddInputPassthrough : public UndoCommand
{
public:
  NodeGroupAddInputPassthrough(NodeGroup *group, const NodeInput &input) :
    group_(group),
    input_(input),
    actually_added_(false)
  {}

  virtual Project * GetRelevantProject() const override
  {
    return group_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  NodeGroup *group_;

  NodeInput input_;

  bool actually_added_;

};

class NodeGroupSetOutputPassthrough : public UndoCommand
{
public:
  NodeGroupSetOutputPassthrough(NodeGroup *group, Node *output) :
    group_(group),
    new_output_(output)
  {}

  virtual Project * GetRelevantProject() const override
  {
    return group_->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  NodeGroup *group_;

  Node *new_output_;
  Node *old_output_;

};

}

#endif // NODEGROUP_H

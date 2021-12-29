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

  void AddInputPassthrough(const NodeInput &input);

  void RemoveInputPassthrough(const NodeInput &input);

  Node *GetOutputPassthrough() const
  {
    return output_passthrough_;
  }

  void SetOutputPassthrough(Node *node);

  static QString GetGroupInputIDFromInput(const NodeInput &input);

  const QHash<QString, NodeInput> &GetInputPassthroughs() const
  {
    return input_passthroughs_;
  }

  bool ContainsInputPassthrough(const NodeInput &input) const;

  virtual QString GetInputName(const QString& id) const override;

signals:
  void InputPassthroughAdded(NodeGroup *group, const NodeInput &input);

  void InputPassthroughRemoved(NodeGroup *group, const NodeInput &input);

  void OutputPassthroughChanged(NodeGroup *group, Node *output);

protected:
  virtual bool LoadCustom(QXmlStreamReader* reader, XMLNodeData& xml_node_data, uint version, const QAtomicInt* cancelled) override;

  virtual void SaveCustom(QXmlStreamWriter* writer) const override;

private:
  QHash<QString, NodeInput> input_passthroughs_;

  Node *output_passthrough_;

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

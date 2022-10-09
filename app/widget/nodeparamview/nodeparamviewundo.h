/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef NODEPARAMVIEWUNDO_H
#define NODEPARAMVIEWUNDO_H

#include "node/keyframe.h"
#include "node/node.h"
#include "node/param.h"
#include "undo/undocommand.h"

namespace olive {

class NodeParamSetKeyframingCommand : public UndoCommand
{
public:
  NodeParamSetKeyframingCommand(const NodeInput& input, bool setting);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput input_;
  bool new_setting_;
  bool old_setting_;

};

class NodeParamInsertKeyframeCommand : public UndoCommand
{
public:
  NodeParamInsertKeyframeCommand(Node *node, NodeKeyframe* keyframe);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node* input_;

  NodeKeyframe* keyframe_;

  QObject memory_manager_;

};

class NodeParamRemoveKeyframeCommand : public UndoCommand
{
public:
  NodeParamRemoveKeyframeCommand(NodeKeyframe* keyframe);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  Node* input_;

  NodeKeyframe* keyframe_;

  QObject memory_manager_;

};

class NodeParamSetKeyframeTimeCommand : public UndoCommand
{
public:
  NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational& time);
  NodeParamSetKeyframeTimeCommand(NodeKeyframe* key, const rational& new_time, const rational& old_time);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframe* key_;

  rational old_time_;
  rational new_time_;

};

class NodeParamSetKeyframeValueCommand : public UndoCommand
{
public:
  NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant& value);
  NodeParamSetKeyframeValueCommand(NodeKeyframe* key, const QVariant& new_value, const QVariant& old_value);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframe* key_;

  QVariant old_value_;
  QVariant new_value_;

};

class NodeParamSetStandardValueCommand : public UndoCommand
{
public:
  NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant& value);
  NodeParamSetStandardValueCommand(const NodeKeyframeTrackReference& input, const QVariant& new_value, const QVariant& old_value);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframeTrackReference ref_;

  QVariant old_value_;
  QVariant new_value_;

};

class NodeParamSetSplitStandardValueCommand : public UndoCommand
{
public:
  NodeParamSetSplitStandardValueCommand(const NodeInput& input, const SplitValue& new_value, const SplitValue& old_value) :
    ref_(input),
    old_value_(old_value),
    new_value_(new_value)
  {}

  NodeParamSetSplitStandardValueCommand(const NodeInput& input, const SplitValue& value) :
    NodeParamSetSplitStandardValueCommand(input, value, input.node()->GetSplitStandardValue(input.input()))
  {}

  virtual Project* GetRelevantProject() const override
  {
    return ref_.node()->project();
  }

protected:
  virtual void redo() override
  {
    ref_.node()->SetSplitStandardValue(ref_.input(), new_value_, ref_.element());
  }

  virtual void undo() override
  {
    ref_.node()->SetSplitStandardValue(ref_.input(), old_value_, ref_.element());
  }

private:
  NodeInput ref_;

  SplitValue old_value_;
  SplitValue new_value_;

};

class NodeParamArrayAppendCommand : public UndoCommand
{
public:
  NodeParamArrayAppendCommand(Node* node, const QString& input);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  Node* node_;

  QString input_;

};

class NodeSetValueHintCommand : public UndoCommand
{
public:
  NodeSetValueHintCommand(const NodeInput &input, const Node::ValueHint &hint) :
    input_(input),
    new_hint_(hint)
  {
  }

  NodeSetValueHintCommand(Node *node, const QString &input, int element, const Node::ValueHint &hint) :
    NodeSetValueHintCommand(NodeInput(node, input, element), hint)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return input_.node()->project();
  }

protected:
  virtual void redo() override;

  virtual void undo() override;

private:
  NodeInput input_;

  Node::ValueHint new_hint_;
  Node::ValueHint old_hint_;

};

}

#endif // NODEPARAMVIEWUNDO_H

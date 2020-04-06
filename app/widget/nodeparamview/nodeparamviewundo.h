/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "node/input.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

class NodeParamSetKeyframingCommand : public UndoCommand {
public:
  NodeParamSetKeyframingCommand(NodeInput* input, bool setting, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeInput* input_;
  bool setting_;
};

class NodeParamInsertKeyframeCommand : public UndoCommand {
public:
  NodeParamInsertKeyframeCommand(NodeInput* input, NodeKeyframePtr keyframe, QUndoCommand *parent = nullptr);
  NodeParamInsertKeyframeCommand(NodeInput* input, NodeKeyframePtr keyframe, bool already_done, QUndoCommand *parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeInput* input_;

  NodeKeyframePtr keyframe_;

  bool done_;

};

class NodeParamRemoveKeyframeCommand : public UndoCommand {
public:
  NodeParamRemoveKeyframeCommand(NodeInput* input, NodeKeyframePtr keyframe, QUndoCommand *parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeInput* input_;

  NodeKeyframePtr keyframe_;

};

class NodeParamSetKeyframeTimeCommand : public UndoCommand {
public:
  NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational& time, QUndoCommand* parent = nullptr);
  NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational& new_time, const rational& old_time, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeKeyframePtr key_;

  rational old_time_;
  rational new_time_;

};

class NodeParamSetKeyframeValueCommand : public UndoCommand {
public:
  NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant& value, QUndoCommand* parent = nullptr);
  NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant& new_value, const QVariant& old_value, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeKeyframePtr key_;

  QVariant old_value_;
  QVariant new_value_;

};

class NodeParamSetStandardValueCommand : public UndoCommand {
public:
  NodeParamSetStandardValueCommand(NodeInput* input, int track, const QVariant& value, QUndoCommand* parent = nullptr);
  NodeParamSetStandardValueCommand(NodeInput* input, int track, const QVariant& new_value, const QVariant& old_value, QUndoCommand* parent = nullptr);

protected:
  virtual void redo_internal() override;
  virtual void undo_internal() override;

private:
  NodeInput* input_;
  int track_;

  QVariant old_value_;
  QVariant new_value_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWUNDO_H

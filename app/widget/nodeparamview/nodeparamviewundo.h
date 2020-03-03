#ifndef NODEPARAMVIEWUNDO_H
#define NODEPARAMVIEWUNDO_H

#include "node/input.h"
#include "undo/undocommand.h"

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

#endif // NODEPARAMVIEWUNDO_H

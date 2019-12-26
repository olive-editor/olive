#ifndef NODEPARAMVIEWUNDO_H
#define NODEPARAMVIEWUNDO_H

#include <QUndoCommand>

#include "node/input.h"

class NodeParamSetKeyframingCommand : public QUndoCommand {
public:
  NodeParamSetKeyframingCommand(NodeInput* input, bool setting, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput* input_;
  bool setting_;
};

class NodeParamInsertKeyframeCommand : public QUndoCommand {
public:
  NodeParamInsertKeyframeCommand(NodeInput* input, NodeKeyframePtr keyframe, QUndoCommand *parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput* input_;

  NodeKeyframePtr keyframe_;

};

class NodeParamRemoveKeyframeCommand : public QUndoCommand {
public:
  NodeParamRemoveKeyframeCommand(NodeInput* input, NodeKeyframePtr keyframe, QUndoCommand *parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput* input_;

  NodeKeyframePtr keyframe_;

};

class NodeParamSetKeyframeTimeCommand : public QUndoCommand {
public:
  NodeParamSetKeyframeTimeCommand(NodeKeyframePtr key, const rational& time, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframePtr key_;

  rational old_time_;
  rational new_time_;

};

class NodeParamSetKeyframeValueCommand : public QUndoCommand {
public:
  NodeParamSetKeyframeValueCommand(NodeKeyframePtr key, const QVariant& value, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeKeyframePtr key_;

  QVariant old_value_;
  QVariant new_value_;

};

class NodeParamSetStandardValueCommand : public QUndoCommand {
public:
  NodeParamSetStandardValueCommand(NodeInput* input, const QVariant& value, QUndoCommand* parent = nullptr);

  virtual void redo() override;
  virtual void undo() override;

private:
  NodeInput* input_;

  QVariant old_value_;
  QVariant new_value_;
};

#endif // NODEPARAMVIEWUNDO_H

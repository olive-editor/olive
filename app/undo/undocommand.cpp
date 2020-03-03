#include "undocommand.h"

#include "core.h"

UndoCommand::UndoCommand(QUndoCommand *parent) :
  QUndoCommand(parent)
{
}

void UndoCommand::redo()
{
  redo_internal();

  modified_ = Core::instance()->IsProjectModified();
  Core::instance()->SetProjectModified(true);
}

void UndoCommand::undo()
{
  undo_internal();

  Core::instance()->SetProjectModified(modified_);
}

void UndoCommand::redo_internal()
{
  QUndoCommand::redo();
}

void UndoCommand::undo_internal()
{
  QUndoCommand::undo();
}

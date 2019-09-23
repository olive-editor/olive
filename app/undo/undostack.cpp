#include "undostack.h"

OliveUndoStack olive::undo_stack;

void OliveUndoStack::pushIfHasChildren(QUndoCommand *command)
{
  if (command->childCount() > 0) {
    push(command);
  } else {
    delete command;
  }
}

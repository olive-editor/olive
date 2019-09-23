#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QUndoStack>

class OliveUndoStack : public QUndoStack {
public:
  /**
   * @brief A wrapper for push() that either pushes if the command has children or deletes if not
   *
   * This function takes ownership of `command`, and may delete it so it should never be accessed after this call.
   */
  void pushIfHasChildren(QUndoCommand* command);
};

namespace olive {
/**
 * @brief A static undo stack for undoable commands throughout Olive
 */
extern OliveUndoStack undo_stack;
}

#endif // UNDOSTACK_H

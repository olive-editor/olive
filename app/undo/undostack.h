#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QUndoStack>

namespace olive {
/**
 * @brief A static undo stack for undoable commands throughout Olive
 */
extern QUndoStack undo_stack;
}

#endif // UNDOSTACK_H

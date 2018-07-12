#ifndef UNDO_H
#define UNDO_H

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>

#include "project/clip.h"
#include "project/sequence.h"

extern QUndoStack undo_stack;

#endif // UNDO_H

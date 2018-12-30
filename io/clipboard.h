#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QVector>

class Transition;

#define CLIPBOARD_TYPE_CLIP 0
#define CLIPBOARD_TYPE_EFFECT 1

extern int clipboard_type;
extern QVector<Transition*> clipboard_transitions;
extern QVector<void*> clipboard;
void clear_clipboard();

#endif // CLIPBOARD_H

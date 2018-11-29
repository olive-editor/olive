#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QVector>

#define CLIPBOARD_TYPE_CLIP 0
#define CLIPBOARD_TYPE_EFFECT 1

extern int clipboard_type;
extern QVector<void*> clipboard;

#endif // CLIPBOARD_H

#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

#ifndef QT_DEBUG
#define dout debug_out << "\n"
extern QDebug debug_out;
#else
#define dout qDebug()
#endif

void setup_debug();
void close_debug();

#endif // DEBUG_H

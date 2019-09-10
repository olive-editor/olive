#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

void DebugHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif // DEBUG_H

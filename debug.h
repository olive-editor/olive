#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

void debug_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
const QString& get_debug_str();

#define dout qDebug()

#endif // DEBUG_H

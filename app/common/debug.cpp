#include "debug.h"


void DebugHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  QByteArray localMsg = msg.toLocal8Bit();

  const char* msg_type = "UNKNOWN";
  switch (type) {
  case QtDebugMsg:
    msg_type = "DEBUG";
    break;
  case QtInfoMsg:
    msg_type = "INFO";
    break;
  case QtWarningMsg:
    msg_type = "WARNING";
    break;
  case QtCriticalMsg:
    msg_type = "ERROR";
    break;
  case QtFatalMsg:
    msg_type = "FATAL";
    break;
  }

  fprintf(stderr, "[%s] %s (%s:%u)\n", msg_type, localMsg.constData(), context.function, context.line);

#ifdef Q_OS_WINDOWS
  // Windows still seems to buffer stderr and we want to see debug messages immediately, so here we make sure each line
  // is flushed
  fflush(stderr);
#endif
}

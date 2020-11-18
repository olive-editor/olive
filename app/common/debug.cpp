/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "debug.h"

namespace olive {

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

}

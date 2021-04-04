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

#include "crashhandler.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QString report;

#ifdef Q_OS_WINDOWS
  int num_args;
  LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &num_args);
  if (num_args < 2) {
    LocalFree(args);
    return 1;
  }

  report = QString::fromWCharArray(args[1]);
  LocalFree(args);
#else
  if (argc < 2) {
    return 1;
  }

  report = argv[1];
#endif

  QApplication a(argc, argv);

  olive::CrashHandlerDialog chd(report);
  chd.open();

  return a.exec();
}

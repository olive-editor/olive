/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include <QApplication>
#include <QSurfaceFormat>

#include "core.h"

int main(int argc, char *argv[]) {
  // Create application instance
  QApplication a(argc, argv);

  // Set application metadata
  QCoreApplication::setOrganizationName("olivevideoeditor.org");
  QCoreApplication::setOrganizationDomain("olivevideoeditor.org");
  QCoreApplication::setApplicationName("Olive");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  QGuiApplication::setDesktopFileName("org.olivevideoeditor.Olive");
#endif

  // Set OpenGL display profile (3.2 Core)
  QSurfaceFormat format;
  format.setVersion(3, 2);
  format.setDepthBufferSize(24);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  olive::core.Start();

  int exit_code = a.exec();

  olive::core.Stop();

  return exit_code;
}

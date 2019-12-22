/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

/** \mainpage Olive Video Editor - Code Documentation
 *
 * This documentation is a primarily a developer resource. For information on using Olive, visit the website
 * https://www.olivevideoeditor.org/
 *
 * Use the navigation above to find documentation on classes or source files.
 */

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}

#include <QApplication>
#include <QSurfaceFormat>

#include "core.h"
#include "common/debug.h"

int main(int argc, char *argv[]) {
  av_log_set_level(AV_LOG_QUIET);

  QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

  // Set OpenGL display profile (3.2 Core)
  QSurfaceFormat format;
  format.setVersion(3, 2);
  format.setDepthBufferSize(24);
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);

  // Create application instance
  QApplication a(argc, argv);

  // Set application metadata
  QCoreApplication::setOrganizationName("olivevideoeditor.org");
  QCoreApplication::setOrganizationDomain("olivevideoeditor.org");
  QCoreApplication::setApplicationName("Olive");

  QString app_version = "0.2.0";
#ifdef GITHASH
  // Anything after the hyphen is considered "unimportant" information. Text BEFORE the hyphen is used in version
  // checking project files and config files
  app_version.append("-");
  app_version.append(GITHASH);
#endif

  qInfo() << "Using Qt version:" << qVersion();

  QCoreApplication::setApplicationVersion(app_version);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  QGuiApplication::setDesktopFileName("org.olivevideoeditor.Olive");
#endif

  // Set up debug handler
  qInstallMessageHandler(DebugHandler);

  // Register FFmpeg codecs and filters (deprecated in 4.0+)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(7, 14, 100)
  avfilter_register_all();
#endif

  // Start core
  olive::core.Start();

  // Run application loop and receive exit code
  int exit_code = a.exec();

  // Clear core memory
  olive::core.Stop();

  return exit_code;
}

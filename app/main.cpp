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

#include <csignal>

#include <QApplication>
#include <QCommandLineParser>
#include <QSurfaceFormat>

#include "core.h"
#include "common/commandlineparser.h"
#include "common/debug.h"

#ifdef USE_CRASHPAD
#include "common/crashpadinterface.h"
#endif // USE_CRASHPAD

int main(int argc, char *argv[])
{
  // Set up debug handler
  qInstallMessageHandler(olive::DebugHandler);

  // Generate version string
  QString app_version = APPVERSION;
#ifdef GITHASH
  // Anything after the hyphen is considered "unimportant" information. Text BEFORE the hyphen is used in version
  // checking project files and config files
  app_version.append("-");
  app_version.append(GITHASH);
#endif

  // Set application metadata
  QCoreApplication::setOrganizationName("olivevideoeditor.org");
  QCoreApplication::setOrganizationDomain("olivevideoeditor.org");
  QCoreApplication::setApplicationName("Olive");
  QGuiApplication::setDesktopFileName("org.olivevideoeditor.Olive");

  QCoreApplication::setApplicationVersion(app_version);


  //
  // Parse command line arguments
  //

  olive::Core::CoreParams startup_params;

  CommandLineParser parser;

  const CommandLineParser::Option* help_option =
      parser.AddOption({QStringLiteral("h"), QStringLiteral("-help")},
                       QCoreApplication::translate("main", "Show this help text"));

  const CommandLineParser::Option* version_option =
      parser.AddOption({QStringLiteral("v"), QStringLiteral("-version")},
                       QCoreApplication::translate("main", "Show application version"));

  const CommandLineParser::Option* fullscreen_option =
      parser.AddOption({QStringLiteral("f"), QStringLiteral("-fullscreen")},
                       QCoreApplication::translate("main", "Start in full-screen mode"));

  const CommandLineParser::Option* export_option =
      parser.AddOption({QStringLiteral("x"), QStringLiteral("-export")},
                       QCoreApplication::translate("main", "Export only (No GUI)"));

  const CommandLineParser::Option* ts_option =
      parser.AddOption({QStringLiteral("-ts")},
                       QCoreApplication::translate("main", "Override language with file"),
                       true,
                       QCoreApplication::translate("main", "qm-file"));

  const CommandLineParser::PositionalArgument* project_argument =
      parser.AddPositionalArgument(QStringLiteral("project"),
                                   QCoreApplication::translate("main", "Project to open on startup"));

  parser.Process(argc, argv);

  if (help_option->IsSet()) {
    // Show help
    parser.PrintHelp(argv[0]);
    return 0;
  }

  if (version_option->IsSet()) {
    // Print version
    printf("%s\n", app_version.toUtf8().constData());
    return 0;
  }

  if (export_option->IsSet()) {
    startup_params.set_run_mode(olive::Core::CoreParams::kHeadlessExport);
  }

  if (ts_option->IsSet()) {
    if (ts_option->GetSetting().isEmpty()) {
      qWarning() << "--ts was set but no translation file was provided";
    } else {
      startup_params.set_startup_language(ts_option->GetSetting());
    }
  }

  startup_params.set_fullscreen(fullscreen_option->IsSet());

  startup_params.set_startup_project(project_argument->GetSetting());

  // Set OpenGL display profile
  QSurfaceFormat format;

  // Tries to cover all bases. If drivers don't support 3.2, they should fallback to the closest
  // alternative. Unfortunately Qt doesn't support 3.0-3.1 without DeprecatedFunctions, so we
  // declare that too. We also force Qt to not use ANGLE because I've had a lot of problems with it
  // so far.
  //
  // https://bugreports.qt.io/browse/QTBUG-46140
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  format.setVersion(3, 2);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DeprecatedFunctions);

  format.setDepthBufferSize(24);
  QSurfaceFormat::setDefaultFormat(format);

  // Enable application automatically using higher resolution images from icons
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  // Create application instance
  std::unique_ptr<QCoreApplication> a;

  if (startup_params.run_mode() == olive::Core::CoreParams::kRunNormal) {
    a.reset(new QApplication(argc, argv));
  } else {
    a.reset(new QCoreApplication(argc, argv));
  }

  // Register FFmpeg codecs and filters (deprecated in 4.0+)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(7, 14, 100)
  avfilter_register_all();
#endif

  // Enable Google Crashpad if compiled with it
#ifdef USE_CRASHPAD
  if (!InitializeCrashpad()) {
    qWarning() << "Failed to initialize Crashpad handler";
  }
#endif // USE_CRASHPAD

  // Start core
  olive::Core c(startup_params);

  c.Start();

  int ret = a->exec();

  // Clear core memory
  c.Stop();

  return ret;
}

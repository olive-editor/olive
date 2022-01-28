/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QMessageBox>
#include <QSurfaceFormat>

#include "core.h"
#include "common/commandlineparser.h"
#include "common/debug.h"
#include "version.h"

#ifdef _WIN32
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <Windows.h>
#endif

#ifdef USE_CRASHPAD
#include "common/crashpadinterface.h"
#endif // USE_CRASHPAD

int main(int argc, char *argv[])
{
  // Set up debug handler
  qInstallMessageHandler(olive::DebugHandler);

  // Set application metadata
  QCoreApplication::setOrganizationName("olivevideoeditor.org");
  QCoreApplication::setOrganizationDomain("olivevideoeditor.org");
  QCoreApplication::setApplicationName("Olive");
  QGuiApplication::setDesktopFileName("org.olivevideoeditor.Olive");
  QCoreApplication::setApplicationVersion(olive::kAppVersionLong);


  //
  // Parse command line arguments
  //

  QVector<QString> args;
#if defined(_WIN32) && defined(UNICODE)
  int wargc;
  LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  args.resize(wargc);
  for (int i=0; i<wargc; i++) {
    args[i] = QString::fromWCharArray(wargv[i]);
  }
  LocalFree(wargv);
#else
  args.resize(argc);
  for (int i=0; i<argc; i++) {
    args[i] = QString::fromLocal8Bit(argv[i]);
  }
#endif

  olive::Core::CoreParams startup_params;

  CommandLineParser parser;

  // Our options
  auto help_option =
      parser.AddOption({QStringLiteral("h"), QStringLiteral("-help")},
                       QCoreApplication::translate("main", "Show this help text"));

  auto version_option =
      parser.AddOption({QStringLiteral("v"), QStringLiteral("-version")},
                       QCoreApplication::translate("main", "Show application version"));

  auto fullscreen_option =
      parser.AddOption({QStringLiteral("f"), QStringLiteral("-fullscreen")},
                       QCoreApplication::translate("main", "Start in full-screen mode"));

  auto export_option =
      parser.AddOption({QStringLiteral("x"), QStringLiteral("-export")},
                       QCoreApplication::translate("main", "Export only (No GUI)"));

  auto ts_option =
      parser.AddOption({QStringLiteral("-ts")},
                       QCoreApplication::translate("main", "Override language with file"),
                       true,
                       QCoreApplication::translate("main", "qm-file"));

#ifdef _WIN32
  auto console_option =
      parser.AddOption({QStringLiteral("c"), QStringLiteral("-console")},
                       QCoreApplication::translate("main", "Launch with debug console"));
#endif // _WIN32

  auto project_argument =
      parser.AddPositionalArgument(QStringLiteral("project"),
                                   QCoreApplication::translate("main", "Project to open on startup"));


  // Qt options re-implemented (add to this as necessary)
  //
  // Because we don't use QCommandLineParser, we must filter out Qt's arguments ourselves. Here,
  // we create them so they're recognized, but never use and also hide them in the "help" text.
  parser.AddOption({QStringLiteral("platform")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("platformpluginpath")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("platformtheme")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("plugin")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("qmljsdebugger")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("qwindowgeometry")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("qwindowicon")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("qwindowtitle")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("reverse")}, QString(), false, QString(), true);
  parser.AddOption({QStringLiteral("session")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("style")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("stylesheet")}, QString(), true, QString(), true);
  parser.AddOption({QStringLiteral("widgetcount")}, QString(), false, QString(), true);

  // Hidden crash option for debugging the crash handling
  auto crash_option = parser.AddOption({QStringLiteral("-crash")}, QString(), true, QString(), true);

  parser.Process(args);

  if (help_option->IsSet()) {
    // Show help
    parser.PrintHelp(argv[0]);
    return 0;
  }

  if (version_option->IsSet()) {
    // Print version
    printf("%s\n", QCoreApplication::applicationVersion().toUtf8().constData());
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

  if (crash_option->IsSet()) {
    startup_params.set_crash_on_startup(true);
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
  format.setVersion(2, 0);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setOption(QSurfaceFormat::DeprecatedFunctions);

  format.setDepthBufferSize(24);
  QSurfaceFormat::setDefaultFormat(format);

  // Enable application automatically using higher resolution images from icons
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

  // Create application instance
  std::unique_ptr<QCoreApplication> a;

  if (startup_params.run_mode() == olive::Core::CoreParams::kRunNormal) {
#ifdef _WIN32
    // Since Olive is linked with the console subsystem (for better POSIX compatibility), a console
    // is created by default. If the user didn't request one, we free it here.
    if (!console_option->IsSet()) {
      FreeConsole();
    }
#endif // _WIN32

    a.reset(new QApplication(argc, argv));
  } else {
    a.reset(new QCoreApplication(argc, argv));
  }

#ifdef _WIN32
  // On Windows, users seem to frequently run into a crash caused by their graphics driver not
  // supporting framebuffers, which we require. I personally have only been able to recreate this
  // when no driver is installed (e.g. when using the Microsoft Basic Display Adapter). Whether
  // that's true for all users or not is still up in the air, but what we do know is it's a driver
  // issue and users should know what to do rather than simply receive a cryptic crash report.
  QOpenGLContext ctx;
  ctx.create();
  QOffscreenSurface surface;
  surface.create();
  ctx.makeCurrent(&surface);
  bool has_proc_address = wglGetProcAddress("glGenFramebuffers");
  std::string gpu_vendor = reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_VENDOR));
  std::string gpu_renderer = reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_RENDERER));
  std::string gpu_version = reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_VERSION));
  ctx.doneCurrent();
  surface.destroy();

  if (!has_proc_address) {
    QString msg = QCoreApplication::translate("main",
      "Your computer's graphics driver does not appear to support framebuffers. "
      "This most likely means either your graphics driver is not up-to-date or your graphics card is too old to run Olive.\n\n"
      "Please update your graphics driver to the latest version and try again.\n\n"
      "Current driver information: %1 %2 %3").arg(QString::fromStdString(gpu_vendor), QString::fromStdString(gpu_renderer), QString::fromStdString(gpu_version));

    if (dynamic_cast<QGuiApplication*>(a.get())) {
      QMessageBox::critical(nullptr, QString(), msg);
    } else {
      qCritical().noquote() << msg;
    }

    return 1;
  }
#endif

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

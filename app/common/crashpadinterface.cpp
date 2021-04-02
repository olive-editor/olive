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

#include "crashpadinterface.h"

#ifdef USE_CRASHPAD

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

#include "crashpadutils.h"
#include "filefunctions.h"

#ifdef OS_WIN
#include <Windows.h>
#endif

crashpad::CrashpadClient *client;

QString report_dialog;
QString report_path;

#if defined(OS_LINUX)
bool ExceptionHandler(int, siginfo_t*, ucontext_t*)
#elif defined(OS_WIN)
LONG WINAPI ExceptionHandler(_EXCEPTION_POINTERS *ExceptionInfo)
#elif defined(OS_APPLE)
#include <signal.h>
QMap<int, void(*)(int)> old_signals;
void SetSignalHandler(int signum, void(*)(int) handler){old_signals.insert(signum, signal(signum, handler));}
void ExceptionHandler(int signum)
#endif
{
  QProcess::startDetached(report_dialog, {report_path, QString::number(QDateTime::currentSecsSinceEpoch()-10)});

#if defined(OS_LINUX)
  return false;
#elif defined(OS_WIN)
  client->DumpAndCrash(ExceptionInfo);
  return EXCEPTION_CONTINUE_SEARCH;
#elif defined(OS_APPLE)
  void(*follow)(int) = old_signals.value(signum);

  if (follow) {
    follow(signum);
  }

  exit(EXIT_FAILURE);
#endif
}

bool InitializeCrashpad()
{
  report_path = QDir(olive::FileFunctions::GetTempFilePath()).filePath(QStringLiteral("reports"));

  QString handler_fn = olive::FileFunctions::GetFormattedExecutableForPlatform(QStringLiteral("crashpad_handler"));

  // Generate absolute path
  QString handler_abs_path = QDir(QCoreApplication::applicationDirPath()).filePath(handler_fn);

  bool status = false;

  if (QFileInfo::exists(handler_abs_path)) {
    base::FilePath handler(QSTRING_TO_BASE_STRING(handler_abs_path));

    base::FilePath reports_dir(QSTRING_TO_BASE_STRING(report_path));

    base::FilePath metrics_dir(QSTRING_TO_BASE_STRING(QDir(olive::FileFunctions::GetTempFilePath()).filePath(QStringLiteral("metrics"))));

    // Metadata that will be posted to the server with the crash report map
    std::map<std::string, std::string> annotations;

    // Disable crashpad rate limiting so that all crashes have dmp files
    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");
    arguments.push_back("--no-upload-gzip");

    // Initialize Crashpad database
    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(reports_dir);
    if (database == NULL) return false;

    // Disable automated crash uploads
    crashpad::Settings *settings = database->GetSettings();
    if (settings == NULL) return false;
    settings->SetUploadsEnabled(false);

    // Start crash handler
    client = new crashpad::CrashpadClient();
    status = client->StartHandler(handler, reports_dir, metrics_dir,
                                  "https://olivevideoeditor.org/crashpad/report.php",
                                  annotations, arguments, true, true);
  }


  // Override Crashpad exception filter with our own
  if (status) {
#if defined(OS_WIN)
    SetUnhandledExceptionFilter(ExceptionHandler);
#elif defined(OS_LINUX)
    crashpad::CrashpadClient::SetFirstChanceExceptionHandler(ExceptionHandler);
#elif defined(OS_APPLE)
    SetSignalHandler(SIGILL, ExceptionHandler);
    SetSignalHandler(SIGFPE, ExceptionHandler);
    SetSignalHandler(SIGSEGV, ExceptionHandler);
    SetSignalHandler(SIGABRT, ExceptionHandler);
    SetSignalHandler(SIGBUS, ExceptionHandler);
#endif
    report_dialog = QDir(qApp->applicationDirPath()).filePath(olive::FileFunctions::GetFormattedExecutableForPlatform(QStringLiteral("olive-crashhandler")));
  } else {
    qWarning() << "Failed to start Crashpad, automatic crash reporting will be disabled";
  }

  return status;
}

#endif // USE_CRASHPAD

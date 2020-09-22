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

QString GenerateReportPath()
{
  return QDir(OLIVE_NAMESPACE::FileFunctions::GetTempFilePath()).filePath(QStringLiteral("reports"));
}

base::FilePath GenerateReportPathForCrashpad()
{
  return base::FilePath(QSTRING_TO_BASE_STRING(GenerateReportPath()));
}

#if defined(OS_WIN)
LONG WINAPI Win32ExceptionHandler(_EXCEPTION_POINTERS *ExceptionInfo)
{
  QString crash_handler_exe = QDir(qApp->applicationDirPath()).filePath(QStringLiteral("olive-crashhandler"));
  QProcess::startDetached(crash_handler_exe, {GenerateReportPath(), QString::number(QDateTime::currentSecsSinceEpoch())});

  client->DumpAndCrash(ExceptionInfo);

  return EXCEPTION_CONTINUE_SEARCH;
}
#elif defined(OS_LINUX)
bool LinuxExceptionHandler(int, siginfo_t*, ucontext_t*)
{
  QString crash_handler_exe = QDir(qApp->applicationDirPath()).filePath(QStringLiteral("olive-crashhandler"));
  QProcess::startDetached(crash_handler_exe, {GenerateReportPath(), QString::number(QDateTime::currentSecsSinceEpoch())});

  // Returning false signals to Crashpad to proceed with its own exception handling
  return false;
}
#endif

bool InitializeCrashpad()
{
  QString exe_dir = QCoreApplication::applicationDirPath();

#ifdef OS_WIN
  base::FilePath handler(QSTRING_TO_BASE_STRING(QDir(exe_dir).filePath(QStringLiteral("crashpad_handler.exe"))));
#else
  // FIXME: On Linux, probably should put this in a subdir so that it doesn't conflict with
  //        anything else in /usr/bin
  base::FilePath handler(QSTRING_TO_BASE_STRING(QDir(exe_dir).filePath(QStringLiteral("crashpad_handler"))));
#endif

  base::FilePath reports_dir = GenerateReportPathForCrashpad();

  base::FilePath metrics_dir(QSTRING_TO_BASE_STRING(QDir(OLIVE_NAMESPACE::FileFunctions::GetTempFilePath()).filePath(QStringLiteral("metrics"))));

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
  bool status = client->StartHandler(handler, reports_dir, metrics_dir,
                                     "https://olivevideoeditor.org/crashpad/report.php",
                                     annotations, arguments, true, true);

  // Override Crashpad exception filter with our own
#if defined(OS_WIN)
  SetUnhandledExceptionFilter(Win32ExceptionHandler);
#elif defined(OS_LINUX)
  crashpad::CrashpadClient::SetFirstChanceExceptionHandler(LinuxExceptionHandler);
#endif

  return status;
}

#endif // USE_CRASHPAD

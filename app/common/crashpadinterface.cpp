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
#include <QDebug>
#include <QDir>
#include <QMessageBox>

#include "filefunctions.h"

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif

// Copied from base::FilePath to match its macro
#if defined(OS_POSIX)
// On most platforms, native pathnames are char arrays, and the encoding
// may or may not be specified.  On Mac OS X, native pathnames are encoded
// in UTF-8.
#define TO_BASE_STRING_TYPE(x) x.toStdString()
#elif defined(OS_WIN)
// On Windows, for Unicode-aware applications, native pathnames are wchar_t
// arrays encoded in UTF-16.
#define TO_BASE_STRING_TYPE(x) x.toStdWString()
#endif  // OS_WIN

crashpad::CrashpadClient *client;

bool ShowCrashConfirmation()
{
  QString msg = QCoreApplication::translate("CrashReport",
                                            "We're sorry, Olive has crashed. "
                                            "Would you like to send an error report to "
                                            "help developers fix this issue?\n\n"
                                            "Crash reports are anonymous and only send "
                                            "non-specific details about your computer and"
                                            "how the crash occurred.");

  return (QMessageBox::critical(nullptr,
                                QString(),
                                msg,
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
}

#ifdef Q_OS_WINDOWS
LONG WINAPI Win32ExceptionHandler(_EXCEPTION_POINTERS *ExceptionInfo)
{
  if (ShowCrashConfirmation()) {
    client->DumpAndCrash(ExceptionInfo);
  }

  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

bool InitializeCrashpad()
{
  QString exe_dir = QCoreApplication::applicationDirPath();

  // FIXME: On Linux, probably should put this in a subdir so that it doesn't conflict with
  //        anything else in /usr/bin

#ifdef Q_OS_WINDOWS
  base::FilePath handler(TO_BASE_STRING_TYPE(QDir(exe_dir).filePath(QStringLiteral("crashpad_handler.exe"))));
#else
  base::FilePath handler(TO_BASE_STRING_TYPE(QDir(exe_dir).filePath(QStringLiteral("crashpad_handler"))));
#endif

  base::FilePath reports_dir(TO_BASE_STRING_TYPE(QDir(OLIVE_NAMESPACE::FileFunctions::GetTempFilePath()).filePath("reports")));

  base::FilePath metrics_dir(TO_BASE_STRING_TYPE(QDir(OLIVE_NAMESPACE::FileFunctions::GetTempFilePath()).filePath("metrics")));

  std::string url = "https://olivevideoeditor.org/crashpad/report.php";

  // Metadata that will be posted to the server with the crash report map
  std::map<std::string, std::string> annotations;

  // Disable crashpad rate limiting so that all crashes have dmp files
  std::vector<std::string> arguments;
  arguments.push_back("--no-rate-limit");
  arguments.push_back("--no-upload-gzip");

  // Initialize Crashpad database
  std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(reports_dir);
  if (database == NULL) return false;

  // Enable automated crash uploads
  crashpad::Settings *settings = database->GetSettings();
  if (settings == NULL) return false;
  settings->SetUploadsEnabled(true);

  // Start crash handler
  client = new crashpad::CrashpadClient();
  bool status = client->StartHandler(handler, reports_dir, metrics_dir,
                                     url, annotations, arguments, true, true);

  // Override Crashpad exception filter with our own
#ifdef Q_OS_WINDOWS
  SetUnhandledExceptionFilter(Win32ExceptionHandler);
#endif

  return status;
}

#endif // USE_CRASHPAD

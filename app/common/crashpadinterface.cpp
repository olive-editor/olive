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
#include <QMap>
#include <QMessageBox>
#include <QProcess>
#include <signal.h>

#include "crashpadutils.h"
#include "filefunctions.h"

#ifdef OS_WIN
#include <Windows.h>
#endif

crashpad::CrashpadClient *client;

QString GenerateReportPath()
{
  return QDir(olive::FileFunctions::GetTempFilePath()).filePath(QStringLiteral("reports"));
}

base::FilePath GenerateReportPathForCrashpad()
{
  return base::FilePath(QSTRING_TO_BASE_STRING(GenerateReportPath()));
}

QMap<int, struct sigaction> old_actions;

void StartCrashReportDialog(int signum)
{
  QString crash_handler_exe = QDir(qApp->applicationDirPath()).filePath(olive::FileFunctions::GetFormattedExecutableForPlatform(QStringLiteral("olive-crashhandler")));
  QProcess::startDetached(crash_handler_exe, {GenerateReportPath(), QString::number(QDateTime::currentSecsSinceEpoch())});

  if (old_actions.value(signum).sa_handler) {
    old_actions.value(signum).sa_handler(signum);
  }
}

void HandleSignal(int signal)
{
  struct sigaction new_action, old_action;

  new_action.sa_handler = StartCrashReportDialog;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction(signal, &new_action, &old_action);
  old_actions.insert(signal, old_action);
}

bool InitializeCrashpad()
{
  QString handler_fn = olive::FileFunctions::GetFormattedExecutableForPlatform(QStringLiteral("crashpad_handler"));

  // Generate absolute path
  QString handler_abs_path = QDir(QCoreApplication::applicationDirPath()).filePath(handler_fn);

  bool status = false;

  if (QFileInfo::exists(handler_abs_path)) {
    base::FilePath handler(QSTRING_TO_BASE_STRING(handler_abs_path));

    base::FilePath reports_dir = GenerateReportPathForCrashpad();

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


  // Add our own signal to show our crash report dialog
  if (status) {
    HandleSignal(SIGILL);
    HandleSignal(SIGFPE);
    HandleSignal(SIGSEGV);
    HandleSignal(SIGTERM);
    HandleSignal(SIGABRT);
#ifndef OS_WIN
    HandleSignal(SIGBUS);
#endif
  } else {
    qWarning() << "Failed to start Crashpad, automatic crash reporting will be disabled";
  }

  return status;
}

#endif // USE_CRASHPAD

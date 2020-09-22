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

#include "crashhandler.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QHttpMultiPart>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include "common/crashpadutils.h"

OLIVE_NAMESPACE_ENTER

CrashHandlerDialog::CrashHandlerDialog(const char *report_dir, const char* crash_time)
{
  setWindowTitle(tr("Olive"));
  crash_time_ = QString(crash_time).toULongLong();
  report_dir_ = report_dir;

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("We're sorry, Olive has crashed. Please help us fix it by "
                                  "sending an error report.")));

  summary_edit_ = new QTextEdit();
  summary_edit_->setPlaceholderText(tr("Describe what you were doing in as much detail as "
                                      "possible. If you can, provide steps to reproduce this crash."));

  layout->addWidget(summary_edit_);

  layout->addWidget(new QLabel(tr("Crash Report:")));

  crash_report_ = new QTextEdit();
  crash_report_->setReadOnly(true);
  crash_report_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  layout->addWidget(crash_report_);

  QHBoxLayout* btn_layout = new QHBoxLayout();
  btn_layout->setMargin(0);
  btn_layout->addStretch();

  send_report_btn_ = new QPushButton(tr("Send Error Report"));
  connect(send_report_btn_, &QPushButton::clicked, this, &CrashHandlerDialog::SendErrorReport);
  btn_layout->addWidget(send_report_btn_);

  dont_send_btn_ = new QPushButton(tr("Don't Send"));
  connect(dont_send_btn_, &QPushButton::clicked, this, &CrashHandlerDialog::reject);
  btn_layout->addWidget(dont_send_btn_);

  layout->addLayout(btn_layout);

  crash_report_->setEnabled(false);
  send_report_btn_->setEnabled(false);

  crash_report_->setText(tr("Waiting for crash report to be generated..."));

  AttemptToFindReport();
}

void CrashHandlerDialog::SetGUIObjectsEnabled(bool e)
{
  summary_edit_->setEnabled(e);
  crash_report_->setEnabled(e);
  send_report_btn_->setEnabled(e);
  dont_send_btn_->setEnabled(e);
}

void CrashHandlerDialog::GenerateReport()
{
  QProcess* p = new QProcess();

  connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &CrashHandlerDialog::ReadProcessFinished);
  connect(p, &QProcess::readyReadStandardOutput, this, &CrashHandlerDialog::ReadProcessHasData);

  QString stackwalk_filename;

#if defined(OS_WIN)
  stackwalk_filename = QStringLiteral("minidump_stackwalk.exe");
#else
  stackwalk_filename = QStringLiteral("minidump_stackwalk");
#endif

  QString stackwalk_bin = QDir(qApp->applicationDirPath()).filePath(stackwalk_filename);
  p->start(stackwalk_bin, {report_filename_});
  crash_report_->setText(QStringLiteral("Trying to run: %1").arg(stackwalk_bin));
}

void CrashHandlerDialog::ReplyFinished(QNetworkReply* reply)
{
  if (reply->error() == QNetworkReply::NoError) {
    // Close dialog
    QDialog::accept();
  } else {
    QMessageBox::critical(this, tr("Upload Failed"),
                          tr("Failed to send error report. Please try again later."),
                          QMessageBox::Ok);
    SetGUIObjectsEnabled(true);
  }
}

void CrashHandlerDialog::AttemptToFindReport()
{
  // Retrieve reports from Crashpad database
  std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(base::FilePath(QSTRING_TO_BASE_STRING(QString(report_dir_))));
  std::vector<crashpad::CrashReportDatabase::Report> reports;
  database->GetCompletedReports(&reports);

  // Find report that was made after the crash time
  foreach (const crashpad::CrashReportDatabase::Report& report, reports) {
    if (report.creation_time >= crash_time_) {
      report_filename_ = BASE_STRING_TO_QSTRING(report.file_path.value());
      break;
    }
  }

  // If we found it, use it, otherwise wait a second and try again
  if (report_filename_.isEmpty()) {
    // Couldn't find report, try again in one second
    QTimer::singleShot(500, this, &CrashHandlerDialog::AttemptToFindReport);
  } else {
    GenerateReport();
  }
}

void CrashHandlerDialog::ReadProcessHasData()
{
  report_data_.append(static_cast<QProcess*>(sender())->readAllStandardOutput());
}

void CrashHandlerDialog::ReadProcessFinished()
{
  SetGUIObjectsEnabled(true);
  crash_report_->setText(report_data_);
  delete sender();
}

void CrashHandlerDialog::SendErrorReport()
{
  SetGUIObjectsEnabled(false);

  QNetworkAccessManager* manager = new QNetworkAccessManager();
  connect(manager, &QNetworkAccessManager::finished, this, &CrashHandlerDialog::ReplyFinished);

  QNetworkRequest request;
  request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
  request.setUrl(QStringLiteral("https://olivevideoeditor.org/crashpad/report.php"));

  // Create HTTP form
  QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  // Create description section
  QHttpPart desc_part;
  desc_part.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/plain"));
  desc_part.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"description\""));
  desc_part.setBody(summary_edit_->toPlainText().toUtf8());
  multipart->append(desc_part);

  // Create file section
  QHttpPart file_part;
  file_part.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));
  file_part.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"upload_file_minidump\"; filename=\"%1\"")
                      .arg(QFileInfo(report_filename_).fileName()));
  QFile* file = new QFile(report_filename_);
  file->open(QFile::ReadOnly);
  file_part.setBodyDevice(file);
  file->setParent(multipart); // Delete file with multipart
  multipart->append(file_part);

  manager->post(request, multipart);
}

OLIVE_NAMESPACE_EXIT

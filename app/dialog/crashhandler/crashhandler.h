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

#ifndef CRASHHANDLERDIALOG_H
#define CRASHHANDLERDIALOG_H

#include <client/crash_report_database.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QNetworkReply>
#include <QPushButton>
#include <QTextEdit>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class CrashHandlerDialog : public QDialog
{
  Q_OBJECT
public:
  CrashHandlerDialog(const char* report_dir, const char* crash_time);

private:
  void SetGUIObjectsEnabled(bool e);

  void GenerateReport();

  QTextEdit* summary_edit_;

  QTextEdit* crash_report_;

  QPushButton* send_report_btn_;

  QPushButton* dont_send_btn_;

  QString report_filename_;

  time_t crash_time_;

  QString report_dir_;

  QByteArray report_data_;

private slots:
  void ReplyFinished(QNetworkReply *reply);

  void AttemptToFindReport();

  void ReadProcessHasData();

  void ReadProcessFinished();

  void SendErrorReport();

};

OLIVE_NAMESPACE_EXIT

#endif // CRASHHANDLERDIALOG_H

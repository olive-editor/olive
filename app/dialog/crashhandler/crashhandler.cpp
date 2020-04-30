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

#include <QFile>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>

OLIVE_NAMESPACE_ENTER

CrashHandlerDialog::CrashHandlerDialog(const char *log_file)
{
  setWindowTitle(tr("Olive"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("We're sorry, Olive has crashed. Please send the following log to the developers to "
                                  "help resolve this.")));

  QTextEdit* edit = new QTextEdit();
  edit->setReadOnly(true);
  layout->addWidget(edit);

  edit->append(QStringLiteral("Build Environment: %1 (%2)").arg(QSysInfo::buildCpuArchitecture(), QSysInfo::buildAbi()));
  edit->append(QStringLiteral("Run Environment: %1").arg(QSysInfo::currentCpuArchitecture()));
  edit->append(QStringLiteral("Kernel: %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()));
  edit->append(QStringLiteral("System: %1 (%2 %3)").arg(QSysInfo::prettyProductName(), QSysInfo::productType(), QSysInfo::productVersion()));
  edit->append(QStringLiteral("Version: %1").arg(GITHASH);
  edit->append(QString());

  QDialogButtonBox* buttons = new QDialogButtonBox();

  // FIXME: Implement auto-reporting
  //buttons->addButton(tr("Send Error Report"), QDialogButtonBox::AcceptRole);
  //buttons->addButton(tr("Don't Send"), QDialogButtonBox::RejectRole);
  buttons->addButton(QDialogButtonBox::Ok);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  QFile log(log_file);
  if (log.open(QFile::ReadOnly | QFile::Text)) {
    edit->append(log.readAll());

    QMetaObject::invokeMethod(edit->verticalScrollBar(),
                              "setValue",
                              Qt::QueuedConnection,
                              Q_ARG(int, 0));

    log.close();
  }
}

void CrashHandlerDialog::accept()
{
  QDialog::accept();
}

void CrashHandlerDialog::reject()
{
  QDialog::reject();
}

OLIVE_NAMESPACE_EXIT

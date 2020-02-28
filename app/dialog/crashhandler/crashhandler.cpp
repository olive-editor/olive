#include "crashhandler.h"

#include <QFile>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>

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

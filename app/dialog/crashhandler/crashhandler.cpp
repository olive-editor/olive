#include "crashhandler.h"

#include <QFile>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>

CrashHandlerDialog::CrashHandlerDialog(const char *log_file)
{
  setWindowTitle(tr("Olive"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("We're sorry, Olive has crashed. Please send the following log to the developers to "
                                  "help resolve this.")));

  QTextEdit* edit = new QTextEdit();
  layout->addWidget(edit);

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
    edit->setText(log.readAll());
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

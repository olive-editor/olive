#include "crashdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollBar>

CrashDialog* olive::crash_dialog;

CrashDialog::CrashDialog()
{
  resize(480, 640);

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("We're very sorry, Olive has crashed. "
                                  "Please send the following data to developers:")));

  text_edit = new QTextEdit();
  text_edit->setReadOnly(true);
  layout->addWidget(text_edit);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  buttons->setCenterButtons(true);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  layout->addWidget(buttons);

  // Set some default data for the crash report so we do as little as possible when we actually crash
#ifdef GITHASH
  text_edit->append(QString("Version: %1").arg(GITHASH));
#else
  text_edit->append(QString("Version: (unknown)"));
#endif
  text_edit->append(QString("Operating System: %1 %2").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture()));
  text_edit->append(QString("Build ABI: %1").arg(QSysInfo::buildAbi()));
  text_edit->append(QString());
}

void CrashDialog::SetData(int signal, const QStringList& strings)
{
  text_edit->append(QString("Signal: %1").arg(signal));
  text_edit->append(QString());
  for (int i=0;i<strings.size();i++) {
    text_edit->append(strings.at(i));
  }
}

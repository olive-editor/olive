#ifndef CRASHDIALOG_H
#define CRASHDIALOG_H

#include <QDialog>
#include <QTextEdit>

class CrashDialog : public QDialog
{
public:
  CrashDialog();

  void SetData(int signal, const QStringList &strings);

private:
  QTextEdit* text_edit;
};

namespace olive {
  extern CrashDialog* crash_dialog;
}

#endif // CRASHDIALOG_H

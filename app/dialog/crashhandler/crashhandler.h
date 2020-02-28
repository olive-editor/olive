#ifndef CRASHHANDLERDIALOG_H
#define CRASHHANDLERDIALOG_H

#include <QDialog>

class CrashHandlerDialog : public QDialog
{
  Q_OBJECT
public:
  CrashHandlerDialog(const char* log_file);

public slots:
  virtual void accept() override;

  virtual void reject() override;

};

#endif // CRASHHANDLERDIALOG_H

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>

class ProgressDialog : public QDialog
{
  Q_OBJECT
public:
  ProgressDialog(const QString &message, const QString &title, QWidget* parent = nullptr);

public slots:
  void SetProgress(int value);

signals:
  void Cancelled();

private:
  QProgressBar* bar_;

};

#endif // PROGRESSDIALOG_H

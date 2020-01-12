#ifndef LOADSAVEDIALOG_H
#define LOADSAVEDIALOG_H

#include <QDialog>
#include <QProgressBar>

class LoadSaveDialog : public QDialog
{
  Q_OBJECT
public:
  LoadSaveDialog(const QString &message, const QString &title, QWidget* parent = nullptr);

public slots:
  void SetProgress(int value);

signals:
  void Cancelled();

private:
  QProgressBar* bar_;
};

#endif // LOADSAVEDIALOG_H

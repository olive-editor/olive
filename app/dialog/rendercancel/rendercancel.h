#ifndef RENDERCANCELDIALOG_H
#define RENDERCANCELDIALOG_H

#include "dialog/loadsave/loadsave.h"

class RenderCancelDialog : public LoadSaveDialog
{
  Q_OBJECT
public:
  RenderCancelDialog(QWidget* parent = nullptr);

  void RunIfWorkersAreBusy();

  void SetWorkerCount(int count);

  void WorkerStarted();

public slots:
  void WorkerDone();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  void UpdateProgress();

  int busy_workers_;

  int total_workers_;

  int waiting_workers_;

};

#endif // RENDERCANCELDIALOG_H

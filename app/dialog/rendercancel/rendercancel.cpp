#include "rendercancel.h"

RenderCancelDialog::RenderCancelDialog(QWidget *parent) :
  LoadSaveDialog(tr("Waiting for workers to finish..."), tr("Renderer"), parent),
  busy_workers_(0),
  total_workers_(0)
{
}

void RenderCancelDialog::RunIfWorkersAreBusy()
{
  if (busy_workers_ > 0) {
    waiting_workers_ = busy_workers_;

    exec();
  }
}

void RenderCancelDialog::SetWorkerCount(int count)
{
  total_workers_ = count;

  UpdateProgress();
}

void RenderCancelDialog::WorkerStarted()
{
  busy_workers_++;

  UpdateProgress();
}

void RenderCancelDialog::WorkerDone()
{
  busy_workers_--;

  UpdateProgress();
}

void RenderCancelDialog::showEvent(QShowEvent *event)
{
  UpdateProgress();
}

void RenderCancelDialog::UpdateProgress()
{
  if (!total_workers_ || !isVisible()) {
    return;
  }

  SetProgress(qRound(100.0 * static_cast<double>(waiting_workers_ - busy_workers_) / static_cast<double>(waiting_workers_)));

  if (busy_workers_ == 0) {
    accept();
  }
}

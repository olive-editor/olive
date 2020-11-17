/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "rendercancel.h"

OLIVE_NAMESPACE_ENTER

RenderCancelDialog::RenderCancelDialog(QWidget *parent) :
  ProgressDialog(tr("Waiting for workers to finish..."), tr("Renderer"), parent),
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
  QDialog::showEvent(event);
  
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

OLIVE_NAMESPACE_EXIT

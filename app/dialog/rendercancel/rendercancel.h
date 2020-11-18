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

#ifndef RENDERCANCELDIALOG_H
#define RENDERCANCELDIALOG_H

#include "dialog/progress/progress.h"

namespace olive {

class RenderCancelDialog : public ProgressDialog
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

}

#endif // RENDERCANCELDIALOG_H

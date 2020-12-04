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

#include "mainstatusbar.h"

#include <QCoreApplication>

namespace olive {

MainStatusBar::MainStatusBar(QWidget *parent) :
  QStatusBar(parent),
  manager_(nullptr),
  connected_task_(nullptr)
{
  setSizeGripEnabled(false);

  bar_ = new QProgressBar();
  addPermanentWidget(bar_);

  bar_->setMinimum(0);
  bar_->setMaximum(100);
  bar_->setVisible(false);

  showMessage(tr("Welcome to %1 %2").arg(QCoreApplication::applicationName(),
                                         QCoreApplication::applicationVersion()));
}

void MainStatusBar::ConnectTaskManager(TaskManager *manager)
{
  if (manager_) {
    disconnect(manager_, &TaskManager::TaskListChanged, this, &MainStatusBar::UpdateStatus);
  }

  manager_ = manager;

  if (manager_) {
    connect(manager_, &TaskManager::TaskListChanged, this, &MainStatusBar::UpdateStatus);
  }
}

void MainStatusBar::UpdateStatus()
{
  if (!manager_) {
    return;
  }

  if (manager_->GetTaskCount() == 0) {
    clearMessage();
    bar_->setVisible(false);
    bar_->setValue(0);
  } else {
    Task* t = manager_->GetFirstTask();

    if (manager_->GetTaskCount() == 1) {
      showMessage(t->GetTitle());
    } else {
      showMessage(tr("Running %1 background task(s)").arg(manager_->GetTaskCount()));
    }

    bar_->setVisible(true);

    if (connected_task_) {
      disconnect(connected_task_, &Task::ProgressChanged, this, &MainStatusBar::SetProgressBarValue);
      disconnect(connected_task_, &Task::destroyed, this, &MainStatusBar::ConnectedTaskDeleted);
    }

    connected_task_ = t;
    connect(connected_task_, &Task::ProgressChanged, this, &MainStatusBar::SetProgressBarValue);
    connect(connected_task_, &Task::destroyed, this, &MainStatusBar::ConnectedTaskDeleted);
  }
}

void MainStatusBar::SetProgressBarValue(double d)
{
  bar_->setValue(qRound(100.0 * d));
}

void MainStatusBar::ConnectedTaskDeleted()
{
  connected_task_ = nullptr;
}

void MainStatusBar::mouseDoubleClickEvent(QMouseEvent* e)
{
  QStatusBar::mouseDoubleClickEvent(e);

  emit DoubleClicked();
}

}

#include "mainstatusbar.h"

#include <QCoreApplication>

MainStatusBar::MainStatusBar(QWidget *parent) :
  QStatusBar(parent),
  manager_(nullptr)
{
  setSizeGripEnabled(false);

  bar_ = new QProgressBar();
  addPermanentWidget(bar_);

  bar_->setMinimum(0);
  bar_->setMaximum(100);
  bar_->setVisible(false);

  showMessage(tr("Welcome to %1 %2").arg(QCoreApplication::applicationName(), QCoreApplication::applicationVersion()));
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
  } else {
    Task* t = manager_->GetFirstTask();

    if (manager_->GetTaskCount() == 1) {
      showMessage(t->GetTitle());
    } else {
      showMessage(tr("Running %1 background tasks").arg(manager_->GetTaskCount()));
    }

    bar_->setVisible(true);
    connect(t, &Task::ProgressChanged, bar_, &QProgressBar::setValue);
  }
}

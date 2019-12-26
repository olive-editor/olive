/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "taskviewitem.h"

#include <QVBoxLayout>

#include "ui/icons/icons.h"

TaskViewItem::TaskViewItem(QWidget *parent) :
  QFrame(parent),
  task_(nullptr)
{
  // Draw border around this item
  setFrameShape(QFrame::StyledPanel);

  // Create layout
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create header label
  task_name_lbl_ = new QLabel(this);
  layout->addWidget(task_name_lbl_);

  // Create center layout (combines progress bar and a cancel button)
  QHBoxLayout* middle_layout = new QHBoxLayout();
  layout->addLayout(middle_layout);

  // Create progress bar
  progress_bar_ = new QProgressBar(this);
  progress_bar_->setRange(0, 100);
  middle_layout->addWidget(progress_bar_);

  // Create cancel button
  cancel_btn_ = new QPushButton(this);
  cancel_btn_->setIcon(icon::Error);
  middle_layout->addWidget(cancel_btn_);

  // Create status label
  task_status_lbl_ = new QLabel(this);
  layout->addWidget(task_status_lbl_);
}

void TaskViewItem::SetTask(Task *t)
{
  // Check if we already have a task and disconnect from it if so
  if (task_ != nullptr) {
    disconnect(task_, SIGNAL(StatusChanged(Task::Status)), this, SLOT(TaskStatusChange(Task::Status)));
    disconnect(task_, SIGNAL(ProgressChanged(int)), progress_bar_, SLOT(setValue(int)));
    disconnect(task_, SIGNAL(destroyed()), this, SLOT(deleteLater()));
  }

  // Set task
  task_ = t;

  // Set name label to the name (bolded)
  task_name_lbl_->setText(QStringLiteral("<b>%1</b>").arg(task_->text()));

  // Connect to the task
  connect(task_, SIGNAL(StatusChanged(Task::Status)), this, SLOT(TaskStatusChange(Task::Status)));
  connect(task_, SIGNAL(ProgressChanged(int)), progress_bar_, SLOT(setValue(int)));
  connect(task_, SIGNAL(Removed()), this, SLOT(deleteLater()));
  connect(cancel_btn_, SIGNAL(clicked(bool)), task_, SLOT(Cancel()));
}

void TaskViewItem::TaskStatusChange(Task::Status status)
{
  switch (status) {
  case Task::kWaiting:
    task_status_lbl_->setText(tr("Waiting..."));
    progress_bar_->setValue(0);
    cancel_btn_->setEnabled(true);
    break;
  case Task::kWorking:
    task_status_lbl_->setText(tr("Working..."));
    progress_bar_->setValue(0);
    cancel_btn_->setEnabled(true);
    break;
  case Task::kFinished:
    task_status_lbl_->setText(tr("Done"));
    progress_bar_->setValue(100);
    cancel_btn_->setEnabled(false);
    break;
  case Task::kError:
    task_status_lbl_->setText(
        tr("Error: %1").arg(static_cast<Task*>(sender())->error())
    );
    progress_bar_->setValue(0);
    cancel_btn_->setEnabled(false);
    break;
  }
}

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

#include "taskviewitem.h"

#include <QDateTime>
#include <QVBoxLayout>

#include "common/timecodefunctions.h"
#include "ui/icons/icons.h"

namespace olive {

TaskViewItem::TaskViewItem(Task* task, QWidget *parent) :
  QFrame(parent),
  task_(task)
{
  // Draw border around this item
  setFrameShape(QFrame::StyledPanel);

  // Create layout
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create header label
  task_name_lbl_ = new QLabel(this);
  task_name_lbl_->setText(task_->GetTitle());
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

  // Create stack with error label and elapsed/remaining time
  status_stack_ = new QStackedWidget();
  status_stack_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout->addWidget(status_stack_);

  // Create elapsed timer
  elapsed_timer_lbl_ = new ElapsedCounterWidget();
  status_stack_->addWidget(elapsed_timer_lbl_);

  // Create error label
  task_error_lbl_ = new QLabel(this);
  status_stack_->addWidget(task_error_lbl_);

  // Set up elapsed timer
  status_stack_->setCurrentWidget(elapsed_timer_lbl_);
  elapsed_timer_lbl_->Start(task_->GetStartTime());

  // Connect to the task
  connect(task_, &Task::ProgressChanged, this, &TaskViewItem::UpdateProgress);
  connect(cancel_btn_, &QPushButton::clicked, this, [this] { emit TaskCancelled(task_); });
}

void TaskViewItem::Failed()
{
  status_stack_->setCurrentWidget(task_error_lbl_);
  task_error_lbl_->setStyleSheet("color: red");
  task_error_lbl_->setText(tr("Error: %1").arg(task_->GetError()));
}

void TaskViewItem::UpdateProgress(double d)
{
  progress_bar_->setValue(qRound(100.0 * d));
  elapsed_timer_lbl_->SetProgress(d);
}

}

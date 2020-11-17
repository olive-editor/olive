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

#include "elapsedcounterwidget.h"

#include <QDateTime>
#include <QHBoxLayout>

#include "common/timecodefunctions.h"

OLIVE_NAMESPACE_ENTER

ElapsedCounterWidget::ElapsedCounterWidget(QWidget* parent) :
  QWidget(parent),
  last_progress_(0),
  start_time_(0)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(layout->spacing() * 8);
  layout->setMargin(0);

  elapsed_lbl_ = new QLabel();
  layout->addWidget(elapsed_lbl_);

  remaining_lbl_ = new QLabel();
  layout->addWidget(remaining_lbl_);

  elapsed_timer_.setInterval(500);
  connect(&elapsed_timer_, &QTimer::timeout, this, &ElapsedCounterWidget::UpdateTimers);
  UpdateTimers();
}

void ElapsedCounterWidget::SetProgress(double d)
{
  last_progress_ = d;
  UpdateTimers();
}

void ElapsedCounterWidget::Start()
{
  Start(QDateTime::currentMSecsSinceEpoch());
}

void ElapsedCounterWidget::Start(qint64 start_time)
{
  start_time_ = start_time;
  elapsed_timer_.start();
  UpdateTimers();
}

void ElapsedCounterWidget::UpdateTimers()
{
  int64_t elapsed_ms, remaining_ms;

  if (last_progress_ > 0) {
    elapsed_ms = QDateTime::currentMSecsSinceEpoch() - start_time_;

    double ms_per_progress_unit = elapsed_ms / last_progress_;
    double remaining_progress = 1.0 - last_progress_;

    remaining_ms = qRound64(ms_per_progress_unit * remaining_progress);
  } else {
    elapsed_ms = 0;
    remaining_ms = 0;
  }

  elapsed_lbl_->setText(tr("Elapsed: %1").arg(Timecode::TimeToString(elapsed_ms)));
  remaining_lbl_->setText(tr("Remaining: %1").arg(Timecode::TimeToString(remaining_ms)));
}

OLIVE_NAMESPACE_EXIT

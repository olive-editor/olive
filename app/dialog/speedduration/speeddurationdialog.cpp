/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "speeddurationdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>

#include "core.h"

namespace olive {

#define super QDialog

SpeedDurationDialog::SpeedDurationDialog(const QVector<ClipBlock *> &clips, const rational &timebase, QWidget *parent) :
  super(parent),
  clips_(clips),
  timebase_(timebase)
{
  setWindowTitle(tr("Speed/Duration"));

  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Speed:")), row, 0);

  speed_slider_ = new FloatSlider();
  speed_slider_->SetDisplayType(FloatSlider::kPercentage);
  connect(speed_slider_, &FloatSlider::ValueChanged, this, &SpeedDurationDialog::SpeedChanged);
  layout->addWidget(speed_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Duration:")), row, 0);

  dur_slider_ = new RationalSlider();
  dur_slider_->SetTimebase(timebase);
  dur_slider_->SetDisplayType(RationalSlider::kTime);
  connect(dur_slider_, &RationalSlider::ValueChanged, this, &SpeedDurationDialog::DurationChanged);
  layout->addWidget(dur_slider_, row, 1);

  row++;

  link_box_ = new QCheckBox(tr("Link Speed and Duration"));
  link_box_->setChecked(true);
  layout->addWidget(link_box_, row, 0, 1, 2);

  row++;

  ripple_box_ = new QCheckBox(tr("Ripple Trailing Clips"));
  layout->addWidget(ripple_box_, row, 0, 1, 2);

  row++;

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btns->setCenterButtons(true);
  connect(btns, &QDialogButtonBox::accepted, this, &SpeedDurationDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &SpeedDurationDialog::reject);
  layout->addWidget(btns, row, 0, 1, 2);

  // Determine which speed value to use
  start_speed_ = clips.first()->speed();
  start_duration_ = clips.first()->length();
  for (int i=1; i<clips.size(); i++) {
    if (!qIsNaN(start_speed_) && !qFuzzyCompare(start_speed_, clips.at(i)->speed())) {
      // Speed differs per clip
      start_speed_ = qSNaN();
    }

    if (start_duration_ != -1 && clips.at(i)->length() != start_duration_) {
      start_duration_ = -1;
    }
  }

  if (qIsNaN(start_speed_)) {
    speed_slider_->SetTristate();
  } else {
    speed_slider_->SetValue(start_speed_);
  }

  if (start_duration_ == -1) {
    dur_slider_->SetTristate();
  } else {
    dur_slider_->SetValue(start_duration_);
  }
}

void SpeedDurationDialog::accept()
{
  super::accept();

  MultiUndoCommand *command = new MultiUndoCommand();

  // Set speed values
  if (speed_slider_->IsTristate()) {
    if (link_box_->isChecked() && !dur_slider_->IsTristate()) {
      // Automatically determine speed from duration
      foreach (ClipBlock *c, clips_) {
        command->add_child(new SetSpeedCommand(c, GetSpeedAdjustment(c->speed(), c->length(), dur_slider_->GetValue())));
      }
    }
  } else {
    // Set speeds to value of slider
    foreach (ClipBlock *c, clips_) {
      command->add_child(new SetSpeedCommand(c, speed_slider_->GetValue()));
    }
  }

  // Set duration values
  if (ripple_box_->isChecked()) {
    // Determine where and how much we need to ripple
    foreach (ClipBlock *c, clips_) {
      rational new_len = c->length();
      if (dur_slider_->IsTristate()) {
        if (link_box_->isChecked() && !speed_slider_->IsTristate()) {
          new_len = GetLengthAdjustment(c->length(), c->speed(), speed_slider_->GetValue(), timebase_);
        }
      } else {
        new_len = dur_slider_->GetValue();
      }
    }
  }


  Core::instance()->undo_stack()->push(command);
}

rational SpeedDurationDialog::GetLengthAdjustment(const rational &original_length, double original_speed, double new_speed, const rational &timebase)
{
  return Timecode::snap_time_to_timebase(rational::fromDouble(original_length.toDouble() / new_speed * original_speed), timebase);
}

double SpeedDurationDialog::GetSpeedAdjustment(double original_speed, const rational &original_length, const rational &new_length)
{
  return original_speed / new_length.toDouble() * original_length.toDouble();
}

void SpeedDurationDialog::SpeedChanged(double s)
{
  if (!link_box_->isChecked()) {
    return;
  }

  if (start_duration_ == -1) {
    dur_slider_->SetTristate();
  } else {
    dur_slider_->SetValue(GetLengthAdjustment(start_duration_, start_speed_, s, timebase_));
  }
}

void SpeedDurationDialog::DurationChanged(const rational &r)
{
  if (!link_box_->isChecked()) {
    return;
  }

  if (qIsNaN(start_speed_)) {
    speed_slider_->SetTristate();
  } else {
    speed_slider_->SetValue(GetSpeedAdjustment(start_speed_, start_duration_, r));
  }
}

void SpeedDurationDialog::SetSpeedCommand::redo()
{
  old_speed_ = clip_->speed();
  clip_->set_speed(new_speed_);
}

void SpeedDurationDialog::SetSpeedCommand::undo()
{
  clip_->set_speed(old_speed_);
}

}

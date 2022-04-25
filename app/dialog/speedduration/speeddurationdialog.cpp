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
#include <QGroupBox>
#include <QMessageBox>

#include "core.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/timelinewidget/undo/timelineundopointer.h"

namespace olive {

#define super QDialog

SpeedDurationDialog::SpeedDurationDialog(const QVector<ClipBlock *> &clips, const rational &timebase, QWidget *parent) :
  super(parent),
  clips_(clips),
  timebase_(timebase)
{
  setWindowTitle(tr("Speed/Duration"));

  QVBoxLayout *layout = new QVBoxLayout(this);

  {
    QGroupBox *speed_group = new QGroupBox();
    layout->addWidget(speed_group);

    QGridLayout *speed_layout = new QGridLayout(speed_group);

    int row = 0;

    speed_layout->addWidget(new QLabel(tr("Speed:")), row, 0);

    speed_slider_ = new FloatSlider();
    speed_slider_->SetDisplayType(FloatSlider::kPercentage);
    connect(speed_slider_, &FloatSlider::ValueChanged, this, &SpeedDurationDialog::SpeedChanged);
    speed_layout->addWidget(speed_slider_, row, 1);

    row++;

    speed_layout->addWidget(new QLabel(tr("Duration:")), row, 0);

    dur_slider_ = new RationalSlider();
    dur_slider_->SetTimebase(timebase);
    dur_slider_->SetDisplayType(RationalSlider::kTime);
    connect(dur_slider_, &RationalSlider::ValueChanged, this, &SpeedDurationDialog::DurationChanged);
    speed_layout->addWidget(dur_slider_, row, 1);

    row++;

    link_box_ = new QCheckBox(tr("Link Speed and Duration"));
    link_box_->setChecked(true);
    speed_layout->addWidget(link_box_, row, 0, 1, 2);
  }

  reverse_box_ = new QCheckBox(tr("Reverse"));
  layout->addWidget(reverse_box_);

  maintain_audio_pitch_box_ = new QCheckBox(tr("Maintain Audio Pitch"));
  layout->addWidget(maintain_audio_pitch_box_);

  ripple_box_ = new QCheckBox(tr("Ripple Trailing Clips"));
  layout->addWidget(ripple_box_);

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btns->setCenterButtons(true);
  connect(btns, &QDialogButtonBox::accepted, this, &SpeedDurationDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &SpeedDurationDialog::reject);
  layout->addWidget(btns);

  // Determine which speed value to use
  start_speed_ = clips.first()->speed();
  start_duration_ = clips.first()->length();
  start_reverse_ = clips.first()->reverse();
  start_maintain_audio_pitch_ = clips.first()->maintain_audio_pitch();
  for (int i=1; i<clips.size(); i++) {
    ClipBlock *c = clips.at(i);

    if (!qIsNaN(start_speed_) && !qFuzzyCompare(start_speed_, c->speed())) {
      // Speed differs per clip
      start_speed_ = qSNaN();
    }

    if (start_duration_ != -1 && c->length() != start_duration_) {
      start_duration_ = -1;
    }

    // Yes, in theory a bool should only ever be 0 or 1 anyway, but MSVC complained and it is
    // *possible* that a bool could be something else, so this code is safer
    int clip_reverse = c->reverse() ? 1 : 0;
    int clip_maintain_pitch = c->maintain_audio_pitch() ? 1 : 0;
    if (start_reverse_ != -1 && clip_reverse != start_reverse_) {
      start_reverse_ = -1;
    }
    if (start_maintain_audio_pitch_ != -1 && clip_maintain_pitch != start_maintain_audio_pitch_) {
      start_maintain_audio_pitch_ = -1;
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

  if (start_reverse_ == -1) {
    reverse_box_->setTristate();
  } else {
    reverse_box_->setChecked(start_reverse_);
  }

  if (start_maintain_audio_pitch_ == -1) {
    maintain_audio_pitch_box_->setTristate();
  } else {
    maintain_audio_pitch_box_->setChecked(start_maintain_audio_pitch_);
  }
}

void SpeedDurationDialog::accept()
{
  // We haven't implemented rippling yet, so warn the user
  if (ripple_box_->isChecked()) {
    // FIXME: Stub
    if (QMessageBox::information(this, QString(), tr("Rippling is a stub and will not do anything. Do you wish to continue?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
      return;
    }
  }

  MultiUndoCommand *command = new MultiUndoCommand();

  // Set duration values
  foreach (ClipBlock *c, clips_) {
    rational proposed_length = c->length();

    if (dur_slider_->IsTristate()) {
      if (link_box_->isChecked() && !speed_slider_->IsTristate()) {
        proposed_length = GetLengthAdjustment(c->length(), c->speed(), speed_slider_->GetValue(), timebase_);
      }
    } else {
      proposed_length = dur_slider_->GetValue();
    }

    if (proposed_length != c->length()) {
      // Clip length should ideally change, but check if there's "room" to do so
      if (proposed_length > c->length() && c->next()) {
        if (GapBlock *gap = dynamic_cast<GapBlock*>(c->next())) {
          proposed_length = qMin(proposed_length, gap->out() - c->in());
        } else {
          proposed_length = c->length();
        }
      }

      if (proposed_length != c->length()) {
        command->add_child(new BlockTrimCommand(c->track(), c, proposed_length, Timeline::kTrimOut));
      }
    }
  }

  // Set speed values
  if (speed_slider_->IsTristate()) {
    if (link_box_->isChecked() && !dur_slider_->IsTristate()) {
      // Automatically determine speed from duration
      foreach (ClipBlock *c, clips_) {
        command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(c, ClipBlock::kSpeedInput)), GetSpeedAdjustment(c->speed(), c->length(), dur_slider_->GetValue())));
      }
    }
  } else {
    // Set speeds to value of slider
    foreach (ClipBlock *c, clips_) {
      command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(c, ClipBlock::kSpeedInput)), speed_slider_->GetValue()));
    }
  }

  // Set reverse values
  if (!reverse_box_->isTristate()) {
    foreach (ClipBlock *c, clips_) {
      command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(c, ClipBlock::kReverseInput)), reverse_box_->isChecked()));
    }
  }

  // Set reverse values
  if (!maintain_audio_pitch_box_->isTristate()) {
    foreach (ClipBlock *c, clips_) {
      command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(c, ClipBlock::kMaintainAudioPitchInput)), maintain_audio_pitch_box_->isChecked()));
    }
  }

  Core::instance()->undo_stack()->push(command);

  super::accept();
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

}

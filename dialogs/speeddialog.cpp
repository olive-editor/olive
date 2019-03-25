/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "speeddialog.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

#include "timeline/sequence.h"
#include "project/footage.h"
#include "rendering/renderfunctions.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "effects/effect.h"
#include "project/media.h"

SpeedDialog::SpeedDialog(QWidget *parent, QVector<Clip*> clips) : QDialog(parent) {
  setWindowTitle(tr("Speed/Duration"));

  clips_ = clips;

  QVBoxLayout* main_layout = new QVBoxLayout(this);

  QGridLayout* grid = new QGridLayout();
  grid->setSpacing(6);

  grid->addWidget(new QLabel(tr("Speed:"), this), 0, 0);
  percent = new LabelSlider(this);
  percent->SetDecimalPlaces(2);
  percent->SetDisplayType(LabelSlider::Percent);
  percent->SetDefault(1);
  grid->addWidget(percent, 0, 1);

  grid->addWidget(new QLabel(tr("Frame Rate:"), this), 1, 0);
  frame_rate = new LabelSlider(this);
  frame_rate->SetDecimalPlaces(3);
  grid->addWidget(frame_rate, 1, 1);

  grid->addWidget(new QLabel(tr("Duration:"), this), 2, 0);
  duration = new LabelSlider(this);
  duration->SetDisplayType(LabelSlider::FrameNumber);
  duration->SetFrameRate(olive::ActiveSequence->frame_rate);
  grid->addWidget(duration, 2, 1);

  main_layout->addLayout(grid);

  reverse = new QCheckBox(tr("Reverse"), this);
  maintain_pitch = new QCheckBox(tr("Maintain Audio Pitch"), this);
  ripple = new QCheckBox(tr("Ripple Changes"), this);

  main_layout->addWidget(reverse);
  main_layout->addWidget(maintain_pitch);
  main_layout->addWidget(ripple);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->setCenterButtons(true);
  main_layout->addWidget(buttonBox);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  connect(percent, SIGNAL(valueChanged(double)), this, SLOT(percent_update()));
  connect(frame_rate, SIGNAL(valueChanged(double)), this, SLOT(frame_rate_update()));
  connect(duration, SIGNAL(valueChanged(double)), this, SLOT(duration_update()));
}

int SpeedDialog::exec() {
  bool enable_frame_rate = false;
  bool multiple_audio = false;
  maintain_pitch->setEnabled(false);

  double default_frame_rate = qSNaN();
  double current_frame_rate = qSNaN();
  double current_percent = qSNaN();
  long default_length = -1;
  long current_length = -1;

  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    double clip_percent;

    // get default frame rate/percentage
    clip_percent = c->speed().value;
    if (c->track() < 0) {
      bool process_video = true;
      if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
        FootageStream* ms = c->media_stream();
        if (ms != nullptr && ms->infinite_length) {
          process_video = false;
        }
      }

      if (process_video) {
        double media_frame_rate = c->media_frame_rate();

        // get "default" frame rate"
        if (enable_frame_rate) {
          // check if frame rate is equal to default
          if (!qIsNaN(default_frame_rate) && !qFuzzyCompare(media_frame_rate, default_frame_rate)) {
            default_frame_rate = qSNaN();
          }
          if (!qIsNaN(current_frame_rate) && !qFuzzyCompare(media_frame_rate*c->speed().value, current_frame_rate)) {
            current_frame_rate = qSNaN();
          }
        } else {
          default_frame_rate = media_frame_rate;
          current_frame_rate = media_frame_rate*c->speed().value;
        }

        enable_frame_rate = true;
      }
    } else {
      maintain_pitch->setEnabled(true);

      if (!multiple_audio) {
        maintain_pitch->setChecked(c->speed().maintain_audio_pitch);
        multiple_audio = true;
      } else if (!maintain_pitch->isTristate() && maintain_pitch->isChecked() != c->speed().maintain_audio_pitch) {
        maintain_pitch->setCheckState(Qt::PartiallyChecked);
        maintain_pitch->setTristate(true);
      }
    }

    if (i == 0) {
      reverse->setChecked(c->reversed());
    } else if (c->reversed() != reverse->isChecked()) {
      reverse->setTristate(true);
      reverse->setCheckState(Qt::PartiallyChecked);
    }

    // get default length
    long clip_default_length = qRound(c->length() * clip_percent);
    if (i == 0) {
      current_length = c->length();
      default_length = clip_default_length;
      current_percent = clip_percent;
    } else {
      if (current_length != -1 && c->length() != current_length) {
        current_length = -1;
      }
      if (default_length != -1 && clip_default_length != default_length) {
        default_length = -1;
      }
      if (!qIsNaN(current_percent) && !qFuzzyCompare(clip_percent, current_percent)) {
        current_percent = qSNaN();
      }
    }
  }

  frame_rate->SetMinimum(1);
  percent->SetMinimum(0.0001);
  duration->SetMinimum(1);

  frame_rate->setEnabled(enable_frame_rate);
  frame_rate->SetDefault(default_frame_rate);
  frame_rate->SetValue(current_frame_rate);
  percent->SetValue(current_percent);
  duration->SetDefault(default_length);
  duration->SetValue((current_length == -1) ? qSNaN() : current_length);

  return QDialog::exec();
}

void SpeedDialog::percent_update() {
  bool got_fr = false;
  double fr_val = qSNaN();
  long len_val = -1;

  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    // get frame rate
    if (frame_rate->isEnabled() && c->track() < 0) {
      double clip_fr = c->media_frame_rate() * percent->value();
      if (got_fr) {
        if (!qIsNaN(fr_val) && !qFuzzyCompare(fr_val, clip_fr)) {
          fr_val = qSNaN();
        }
      } else {
        fr_val = clip_fr;
        got_fr = true;
      }
    }

    // get duration
    long clip_default_length = qRound(c->length() * c->speed().value);
    long new_clip_length = qRound(clip_default_length / percent->value());
    if (i == 0) {
      len_val = new_clip_length;
    } else if (len_val > -1 && len_val != new_clip_length) {
      len_val = -1;
    }
  }

  frame_rate->SetValue(fr_val);
  duration->SetValue((len_val == -1) ? qSNaN() : len_val);
}

void SpeedDialog::duration_update() {
  double pc_val = qSNaN();
  bool got_fr = false;
  double fr_val = qSNaN();

  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    // get percent
    long clip_default_length = qRound(c->length() * c->speed().value);
    double clip_pc = clip_default_length / duration->value();
    if (i == 0) {
      pc_val = clip_pc;
    } else if (!qIsNaN(pc_val) && !qFuzzyCompare(clip_pc, pc_val)) {
      pc_val = qSNaN();
    }

    // get frame rate
    if (frame_rate->isEnabled() && c->track() < 0) {
      double clip_fr = c->media_frame_rate() * clip_pc;
      if (got_fr) {
        if (!qIsNaN(fr_val) && !qFuzzyCompare(fr_val, clip_fr)) {
          fr_val = qSNaN();
        }
      } else {
        fr_val = clip_fr;
        got_fr = true;
      }
    }
  }

  frame_rate->SetValue(fr_val);
  percent->SetValue(pc_val);
}

void SpeedDialog::frame_rate_update() {
  /*double fr = (frame_rate->value());
  double pc = (fr / default_frame_rate);
  percent->set_value(pc, false);
  duration->set_value(default_length / pc, false);*/

  double old_pc_val = qSNaN();
  bool got_pc_val = false;
  double pc_val = qSNaN();
  bool got_len_val = false;
  long len_val = -1;

  // analyze video clips
  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    // check if all selected clips are currently the same speed
    if (i == 0) {
      old_pc_val = c->speed().value;
    } else if (!qIsNaN(old_pc_val) && !qFuzzyCompare(c->speed().value, old_pc_val)) {
      old_pc_val = qSNaN();
    }

    if (c->track() < 0) {
      // what would the new speed be based on this frame rate
      double new_clip_speed = frame_rate->value() / c->media_frame_rate();
      if (!got_pc_val) {
        pc_val = new_clip_speed;
        got_pc_val = true;
      } else if (!qIsNaN(pc_val) && !qFuzzyCompare(pc_val, new_clip_speed)) {
        pc_val = qSNaN();
      }

      // what would be the new length based on this speed
      long new_clip_len = (c->length() * c->speed().value) / new_clip_speed;
      if (!got_len_val) {
        len_val = new_clip_len;
        got_len_val = true;
      } else if (len_val > -1 && new_clip_len != len_val) {
        len_val = -1;
      }
    }
  }

  // analyze audio clips
  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    if (c->track() >= 0) {

      long new_clip_len = (qIsNaN(old_pc_val) || qIsNaN(pc_val)) ?
            c->length() : qRound((c->length() * c->speed().value) / pc_val);

      if (len_val > -1 && new_clip_len != len_val) {
        len_val = -1;
        break;
      }
    }
  }

  percent->SetValue(pc_val);
  duration->SetValue((len_val == -1) ? qSNaN() : len_val);
}

void set_speed(ComboAction* ca, Clip* c, double speed, bool ripple, long& ep, long& lr) {
  panel_timeline->deselect_area(c->timeline_in(), c->timeline_out(), c->track());

  long proposed_out = c->timeline_out();
  double multiplier = (c->speed().value / speed);
  proposed_out = qRound(c->timeline_in() + (c->length() * multiplier));
  ca->append(new SetSpeedAction(c, speed));
  if (!ripple && proposed_out > c->timeline_out()) {
    for (int i=0;i<c->sequence->clips.size();i++) {
            ClipPtr compare = c->sequence->clips.at(i);
      if (compare != nullptr
          && compare->track() == c->track()
          && compare->timeline_in() >= c->timeline_out() && compare->timeline_in() < proposed_out) {
        proposed_out = compare->timeline_in();
      }
    }
  }
  ep = qMin(ep, c->timeline_out());
  lr = qMax(lr, proposed_out - c->timeline_out());
  c->move(ca, c->timeline_in(), proposed_out, qRound(c->clip_in() * multiplier), c->track());

  c->refactor_frame_rate(ca, multiplier, false);

  Selection sel;
  sel.in = c->timeline_in();
  sel.out = proposed_out;
  sel.track = c->track();
  olive::ActiveSequence->selections.append(sel);
}

void SpeedDialog::accept() {
  ComboAction* ca = new ComboAction();

  // undoable action for setting "maintain audio pitch"
  SetClipProperty* audio_pitch_action = new SetClipProperty(kSetClipPropertyMaintainAudioPitch);

  // undoable action for setting "reversed"
  SetClipProperty* reversed_action = new SetClipProperty(kSetClipPropertyReversed);

  // undoable action for restoring clip selections
  SetSelectionsCommand* sel_command = new SetSelectionsCommand(olive::ActiveSequence.get());
  sel_command->old_data = olive::ActiveSequence->selections;

  // variables used to calculate ripples
  long earliest_point = LONG_MAX;
  long longest_ripple = LONG_MIN;

  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);

    // make sure the clip is closed while we're making changes
    if (c->IsOpen()) {
      c->Close(true);
    }

    // set maintain audio pitch if the user made a selection
    if (c->track() >= 0
        && maintain_pitch->checkState() != Qt::PartiallyChecked
        && c->speed().maintain_audio_pitch != maintain_pitch->isChecked()) {
      audio_pitch_action->AddSetting(c, maintain_pitch->isChecked());
    }

    // set reverse setting if the user made a selection
    if (reverse->checkState() != Qt::PartiallyChecked && c->reversed() != reverse->isChecked()) {
      long new_clip_in = (c->media_length() - (c->length() + c->clip_in()));
      c->move(ca, c->timeline_in(), c->timeline_out(), new_clip_in, c->track());
      c->set_clip_in(new_clip_in);
      reversed_action->AddSetting(c, reverse->isChecked());
    }
  }

  // setting the actual speed
  if (!qIsNaN(percent->value())) {

    // if we have a percentage value, use that on all the clips
    for (int i=0;i<clips_.size();i++) {
      Clip* c = clips_.at(i);
      set_speed(ca, c, percent->value(), ripple->isChecked(), earliest_point, longest_ripple);
    }

  } else if (!qIsNaN(frame_rate->value())) {

    // if the user changed the speed by changing the frame rate,
    bool can_change_all = true;
    double cached_speed = clips_.first()->speed().value;
    double cached_fr = qSNaN();

    // see if we can use the frame rate to change all the speeds
    for (int i=0;i<clips_.size();i++) {
      Clip* c = clips_.at(i);
      if (i > 0 && !qFuzzyCompare(cached_speed, c->speed().value)) {
        can_change_all = false;
      }
      if (c->track() < 0) {
        if (qIsNaN(cached_fr)) {
          cached_fr = c->media_frame_rate();
        } else if (!qFuzzyCompare(cached_fr, c->media_frame_rate())) {
          can_change_all = false;
          break;
        }
      }
    }

    // make changes
    for (int i=0;i<clips_.size();i++) {
      Clip* c = clips_.at(i);
      if (c->track() < 0) {
        set_speed(ca, c, frame_rate->value() / c->media_frame_rate(), ripple->isChecked(), earliest_point, longest_ripple);
      } else if (can_change_all) {
        set_speed(ca, c, frame_rate->value() / cached_fr, ripple->isChecked(), earliest_point, longest_ripple);
      }
    }
  } else if (!qIsNaN(duration->value())) {
    // simply set duration
    for (int i=0;i<clips_.size();i++) {
      Clip* c = clips_.at(i);
      set_speed(ca, c, (c->length() * c->speed().value) / duration->value(), ripple->isChecked(), earliest_point, longest_ripple);
    }
  }

  if (ripple->isChecked()) {
    ripple_clips(ca, clips_.at(0)->sequence, earliest_point, longest_ripple);
  }

  sel_command->new_data = olive::ActiveSequence->selections;
  ca->append(sel_command);

  ca->append(reversed_action);
  ca->append(audio_pitch_action);

  olive::UndoStack.push(ca);

  update_ui(true);
  QDialog::accept();
}

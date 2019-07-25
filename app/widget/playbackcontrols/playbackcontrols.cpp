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

#include "playbackcontrols.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "common/timecodefunctions.h"
#include "ui/icons/icons.h"

PlaybackControls::PlaybackControls(QWidget *parent) :
  QWidget(parent),
  time_base_(0)
{
  // Create lower controls
  QHBoxLayout* lower_control_layout = new QHBoxLayout(this);
  lower_control_layout->setSpacing(0);
  lower_control_layout->setMargin(0);

  QSizePolicy lower_container_size_policy = QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

  // In the lower-left, we create a current timecode label wrapped in a QWidget for fixed sizing
  lower_left_container_ = new QWidget();
  lower_left_container_->setVisible(false);
  lower_left_container_->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_left_container_);

  QHBoxLayout* lower_left_layout = new QHBoxLayout(lower_left_container_);
  lower_left_layout->setSpacing(0);
  lower_left_layout->setMargin(0);

  cur_tc_lbl_ = new QLabel();
  lower_left_layout->addWidget(cur_tc_lbl_);
  lower_left_layout->addStretch();

  // In the lower-middle, we create playback control buttons
  QWidget* lower_middle_container = new QWidget();
  lower_middle_container->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_middle_container);

  QHBoxLayout* lower_middle_layout = new QHBoxLayout(lower_middle_container);
  lower_middle_layout->setSpacing(0);
  lower_middle_layout->setMargin(0);

  lower_middle_layout->addStretch();

  // Go To Start Button
  QPushButton* go_to_start_btn = new QPushButton();
  go_to_start_btn->setIcon(olive::icon::GoToStart);
  lower_middle_layout->addWidget(go_to_start_btn);
  connect(go_to_start_btn, SIGNAL(clicked(bool)), this, SIGNAL(BeginClicked()));

  // Prev Frame Button
  QPushButton* prev_frame_btn = new QPushButton();
  prev_frame_btn->setIcon(olive::icon::PrevFrame);
  lower_middle_layout->addWidget(prev_frame_btn);
  connect(prev_frame_btn, SIGNAL(clicked(bool)), this, SIGNAL(PrevFrameClicked()));

  // Play/Pause Button
  QPushButton* play_btn = new QPushButton();
  play_btn->setIcon(olive::icon::Play);
  lower_middle_layout->addWidget(play_btn);
  connect(play_btn, SIGNAL(clicked(bool)), this, SIGNAL(PlayClicked()));

  // Next Frame Button
  QPushButton* next_frame_btn = new QPushButton();
  next_frame_btn->setIcon(olive::icon::NextFrame);
  lower_middle_layout->addWidget(next_frame_btn);
  connect(next_frame_btn, SIGNAL(clicked(bool)), this, SIGNAL(NextFrameClicked()));

  // Go To End Button
  QPushButton* go_to_end_btn = new QPushButton();
  go_to_end_btn->setIcon(olive::icon::GoToEnd);
  lower_middle_layout->addWidget(go_to_end_btn);
  connect(go_to_end_btn, SIGNAL(clicked(bool)), this, SIGNAL(EndClicked()));

  lower_middle_layout->addStretch();

  // The lower-right, we create another timecode label, this time to show the end timecode
  lower_right_container_ = new QWidget();
  lower_right_container_->setVisible(false);
  lower_right_container_->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_right_container_);

  QHBoxLayout* lower_right_layout = new QHBoxLayout(lower_right_container_);
  lower_right_layout->setSpacing(0);
  lower_right_layout->setMargin(0);

  lower_right_layout->addStretch();
  end_tc_lbl_ = new QLabel("00:00:00;00");
  lower_right_layout->addWidget(end_tc_lbl_);
}

void PlaybackControls::SetTimecodeEnabled(bool enabled)
{
  lower_left_container_->setVisible(enabled);
  lower_right_container_->setVisible(enabled);
}

void PlaybackControls::SetTimebase(const rational &r)
{
  time_base_ = r;
}

void PlaybackControls::SetTime(const rational &r)
{
  Q_ASSERT(time_base_.denominator() != 0);

  cur_tc_lbl_->setText(olive::timestamp_to_timecode(r, time_base_, olive::kTimecodeFrames));
}

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

#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>

#include "core.h"
#include "config/config.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

PlaybackControls::PlaybackControls(QWidget *parent) :
  QWidget(parent),
  time_base_(0)
{
  // Create lower controls
  QHBoxLayout* lower_control_layout = new QHBoxLayout(this);
  lower_control_layout->setSpacing(0);
  lower_control_layout->setMargin(0);

  QSizePolicy lower_container_size_policy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  lower_container_size_policy.setHorizontalStretch(1);

  // In the lower-left, we create a current timecode label wrapped in a QWidget for fixed sizing
  lower_left_container_ = new QWidget();
  lower_left_container_->setVisible(false);
  lower_left_container_->setSizePolicy(lower_container_size_policy);

  lower_control_layout->addWidget(lower_left_container_);

  QHBoxLayout* lower_left_layout = new QHBoxLayout(lower_left_container_);
  lower_left_layout->setSpacing(0);
  lower_left_layout->setMargin(0);

  cur_tc_lbl_ = new TimeSlider();
  connect(cur_tc_lbl_, &TimeSlider::ValueChanged, this, &PlaybackControls::TimeChanged);
  lower_left_layout->addWidget(cur_tc_lbl_);
  lower_left_layout->addStretch();

  // This is only here
  QWidget* blank_widget = new QWidget();
  //new QHBoxLayout(blank_widget);
  blank_widget->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(blank_widget);

  // In the lower-middle, we create playback control buttons
  QWidget* lower_middle_container = new QWidget();
  lower_middle_container->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_middle_container);

  QHBoxLayout* lower_middle_layout = new QHBoxLayout(lower_middle_container);
  lower_middle_layout->setSpacing(0);
  lower_middle_layout->setMargin(0);

  // Go To Start Button
  go_to_start_btn_ = new QPushButton();
  lower_middle_layout->addWidget(go_to_start_btn_);
  connect(go_to_start_btn_, &QPushButton::clicked, this, &PlaybackControls::BeginClicked);

  // Prev Frame Button
  prev_frame_btn_ = new QPushButton();
  lower_middle_layout->addWidget(prev_frame_btn_);
  connect(prev_frame_btn_, &QPushButton::clicked, this, &PlaybackControls::PrevFrameClicked);

  // Play/Pause Button
  playpause_stack_ = new QStackedWidget();
  lower_middle_layout->addWidget(playpause_stack_);

  play_btn_ = new QPushButton();
  playpause_stack_->addWidget(play_btn_);
  connect(play_btn_, &QPushButton::clicked, this, &PlaybackControls::PlayClicked);

  pause_btn_ = new QPushButton();
  playpause_stack_->addWidget(pause_btn_);
  connect(pause_btn_, &QPushButton::clicked, this, &PlaybackControls::PauseClicked);

  // Default to showing play button
  playpause_stack_->setCurrentWidget(play_btn_);

  // Hack to conform the play/pause button size to the other buttons (QStackedWidget has a
  // different size policy by default)
  playpause_stack_->setSizePolicy(prev_frame_btn_->sizePolicy());

  // Next Frame Button
  next_frame_btn_ = new QPushButton();
  lower_middle_layout->addWidget(next_frame_btn_);
  connect(next_frame_btn_, &QPushButton::clicked, this, &PlaybackControls::NextFrameClicked);

  // Go To End Button
  go_to_end_btn_ = new QPushButton();
  lower_middle_layout->addWidget(go_to_end_btn_);
  connect(go_to_end_btn_, &QPushButton::clicked, this, &PlaybackControls::EndClicked);

  QWidget* av_btn_widget = new QWidget();
  av_btn_widget->setSizePolicy(lower_container_size_policy);
  QHBoxLayout* av_btn_layout = new QHBoxLayout(av_btn_widget);
  av_btn_layout->setSpacing(0);
  av_btn_layout->setMargin(0);
  video_drag_btn_ = new DragButton();
  connect(video_drag_btn_, &QPushButton::clicked, this, &PlaybackControls::VideoClicked);
  connect(video_drag_btn_, &DragButton::MousePressed, this, &PlaybackControls::VideoPressed);
  av_btn_layout->addWidget(video_drag_btn_);
  audio_drag_btn_ = new DragButton();
  connect(audio_drag_btn_, &QPushButton::clicked, this, &PlaybackControls::AudioClicked);
  connect(audio_drag_btn_, &DragButton::MousePressed, this, &PlaybackControls::AudioPressed);
  av_btn_layout->addWidget(audio_drag_btn_);
  lower_control_layout->addWidget(av_btn_widget);

  // The lower-right, we create another timecode label, this time to show the end timecode
  lower_right_container_ = new QWidget();
  lower_right_container_->setVisible(false);
  lower_right_container_->setSizePolicy(lower_container_size_policy);
  lower_control_layout->addWidget(lower_right_container_);

  QHBoxLayout* lower_right_layout = new QHBoxLayout(lower_right_container_);
  lower_right_layout->setSpacing(0);
  lower_right_layout->setMargin(0);

  lower_right_layout->addStretch();
  end_tc_lbl_ = new QLabel();
  lower_right_layout->addWidget(end_tc_lbl_);

  UpdateIcons();

  SetTimebase(0);

  SetAudioVideoDragButtonsVisible(false);

  connect(Core::instance(), &Core::TimecodeDisplayChanged, this, &PlaybackControls::TimecodeChanged);
}

void PlaybackControls::SetTimecodeEnabled(bool enabled)
{
  lower_left_container_->setVisible(enabled);
  lower_right_container_->setVisible(enabled);
}

void PlaybackControls::SetTimebase(const rational &r)
{
  time_base_ = r;
  cur_tc_lbl_->SetTimebase(r);

  cur_tc_lbl_->setEnabled(!r.isNull());
}

void PlaybackControls::SetAudioVideoDragButtonsVisible(bool e)
{
  video_drag_btn_->setVisible(e);
  audio_drag_btn_->setVisible(e);
}

void PlaybackControls::SetTime(const int64_t &r)
{
  cur_tc_lbl_->SetValue(r);
}

void PlaybackControls::SetEndTime(const int64_t &r)
{
  if (time_base_.isNull()) {
    return;
  }

  end_time_ = r;

  end_tc_lbl_->setText(Timecode::timestamp_to_timecode(end_time_,
                                                       time_base_,
                                                       Core::instance()->GetTimecodeDisplay()));

  if (end_time_ == 0) {
    end_tc_lbl_->setVisible(false);
  }
}

void PlaybackControls::ShowPauseButton()
{
  // Play was clicked, toggle to pause
  playpause_stack_->setCurrentWidget(pause_btn_);
}

void PlaybackControls::ShowPlayButton()
{
  playpause_stack_->setCurrentWidget(play_btn_);
}

void PlaybackControls::changeEvent(QEvent *e)
{
  QWidget::changeEvent(e);

  if (e->type() == QEvent::StyleChange) {
    UpdateIcons();
  }
}

void PlaybackControls::UpdateIcons()
{
  go_to_start_btn_->setIcon(icon::GoToStart);
  prev_frame_btn_->setIcon(icon::PrevFrame);
  play_btn_->setIcon(icon::Play);
  pause_btn_->setIcon(icon::Pause);
  next_frame_btn_->setIcon(icon::NextFrame);
  go_to_end_btn_->setIcon(icon::GoToEnd);
  video_drag_btn_->setIcon(icon::Video);
  audio_drag_btn_->setIcon(icon::Audio);
}

void PlaybackControls::TimecodeChanged()
{
  // Update end time
  SetEndTime(end_time_);
}

OLIVE_NAMESPACE_EXIT

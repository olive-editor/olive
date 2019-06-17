#include "playbackcontrols.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "ui/icons/icons.h"

PlaybackControls::PlaybackControls(QWidget *parent) :
  QWidget(parent)
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

  cur_tc_lbl_ = new QLabel("00:00:00;00");
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

  // Prev Frame Button
  QPushButton* prev_frame_btn = new QPushButton();
  prev_frame_btn->setIcon(olive::icon::PrevFrame);
  lower_middle_layout->addWidget(prev_frame_btn);

  // Play/Pause Button
  QPushButton* play_btn = new QPushButton();
  play_btn->setIcon(olive::icon::Play);
  lower_middle_layout->addWidget(play_btn);

  // Next Frame Button
  QPushButton* next_frame_btn = new QPushButton();
  next_frame_btn->setIcon(olive::icon::NextFrame);
  lower_middle_layout->addWidget(next_frame_btn);

  // Go To End Button
  QPushButton* go_to_end_btn = new QPushButton();
  go_to_end_btn->setIcon(olive::icon::GoToEnd);
  lower_middle_layout->addWidget(go_to_end_btn);

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

#include "timelinelabel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TimelineLabel::TimelineLabel() :
  track_(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  QLabel* label = new QLabel("Track!");
  layout->addWidget(label);

  mute_button_ = new QPushButton("M");
  mute_button_->setCheckable(true);
  mute_button_->setFlat(true);
  layout->addWidget(mute_button_);

  solo_button_ = new QPushButton("S");
  solo_button_->setCheckable(true);
  solo_button_->setFlat(true);
  layout->addWidget(solo_button_);

  lock_button_ = new QPushButton("L");
  lock_button_->setCheckable(true);
  lock_button_->setFlat(true);
  layout->addWidget(lock_button_);
}

void TimelineLabel::SetTrack(Track *track)
{
  if (track_ != nullptr) {
    disconnect(mute_button_, SIGNAL(toggled(bool)), track_, SLOT(SetMuted(bool)));
    disconnect(solo_button_, SIGNAL(toggled(bool)), track_, SLOT(SetSoloed(bool)));
    disconnect(lock_button_, SIGNAL(toggled(bool)), track_, SLOT(SetLocked(bool)));
  }

  track_ = track;

  if (track != nullptr) {
    mute_button_->setChecked(track->IsMuted());
    solo_button_->setChecked(track->IsSoloed());
    lock_button_->setChecked(track->IsLocked());

    connect(mute_button_, SIGNAL(toggled(bool)), track_, SLOT(SetMuted(bool)));
    connect(solo_button_, SIGNAL(toggled(bool)), track_, SLOT(SetSoloed(bool)));
    connect(lock_button_, SIGNAL(toggled(bool)), track_, SLOT(SetLocked(bool)));
  }
}

#include "timelinelabel.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>

TimelineLabel::TimelineLabel() :
  track_(nullptr),
  alignment_(olive::timeline::kAlignmentTop)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(layout->spacing()/2);

  label_ = new ClickableLabel();
  layout->addWidget(label_);
  connect(label_, SIGNAL(double_clicked()), this, SLOT(RenameTrack()));

  mute_button_ = new QPushButton("M");
  QSize fixed_size(mute_button_->sizeHint().height(), mute_button_->sizeHint().height());
  fixed_size *= 0.75;
  mute_button_->setFixedSize(fixed_size);
  mute_button_->setStyleSheet("QPushButton::checked { background: red; }");
  mute_button_->setCheckable(true);
  layout->addWidget(mute_button_);

  solo_button_ = new QPushButton("S");
  solo_button_->setFixedSize(fixed_size);
  solo_button_->setStyleSheet("QPushButton::checked { background: yellow; }");
  solo_button_->setCheckable(true);
  layout->addWidget(solo_button_);

  lock_button_ = new QPushButton("L");
  lock_button_->setFixedSize(fixed_size);
  lock_button_->setStyleSheet("QPushButton::checked { background: gray; }");
  lock_button_->setCheckable(true);
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

  if (track_ != nullptr) {
    UpdateState();

    connect(mute_button_, SIGNAL(toggled(bool)), track_, SLOT(SetMuted(bool)));
    connect(solo_button_, SIGNAL(toggled(bool)), track_, SLOT(SetSoloed(bool)));
    connect(lock_button_, SIGNAL(toggled(bool)), track_, SLOT(SetLocked(bool)));

    connect(track_, SIGNAL(HeightChanged(int)), this, SLOT(UpdateHeight(int)));
  }
}

void TimelineLabel::UpdateState()
{
  label_->setText(track_->name());

  UpdateHeight(track_->height());

  mute_button_->setChecked(track_->IsMuted());
  solo_button_->setChecked(track_->IsSoloed());
  lock_button_->setChecked(track_->IsLocked());
}

void TimelineLabel::SetAlignment(olive::timeline::Alignment alignment)
{
  alignment_ = alignment;
  update();
}

void TimelineLabel::paintEvent(QPaintEvent *)
{
  if (alignment_ != olive::timeline::kAlignmentTop && alignment_ != olive::timeline::kAlignmentBottom) {
    return;
  }

  QPainter p(this);

  p.setPen(QColor(0, 0, 0, 96));

  int line_y = (alignment_ == olive::timeline::kAlignmentTop) ? height() - 1 : 0;

  p.drawLine(0, line_y, width(), line_y);
}

void TimelineLabel::RenameTrack()
{
  bool ok;
  QString new_name = QInputDialog::getText(this,
                                           tr("Rename Track"),
                                           tr("Enter the new name for this track"),
                                           QLineEdit::Normal,
                                           track_->name(),
                                           &ok);
  if (ok) {
    track_->SetName(new_name);

    UpdateState();
  }
}

void TimelineLabel::UpdateHeight(int h)
{
  setFixedHeight(h+1);
}

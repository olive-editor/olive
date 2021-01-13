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

#include "trackviewitem.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

namespace olive {

TrackViewItem::TrackViewItem(Track* track, QWidget *parent) :
  QWidget(parent),
  track_(track)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  stack_ = new QStackedWidget();
  layout->addWidget(stack_);

  label_ = new ClickableLabel();
  connect(label_, &ClickableLabel::MouseDoubleClicked, this, &TrackViewItem::LabelClicked);
  connect(track_, &Track::LabelChanged, this, &TrackViewItem::UpdateLabel);
  UpdateLabel();
  stack_->addWidget(label_);

  line_edit_ = new FocusableLineEdit();
  connect(line_edit_, &FocusableLineEdit::Confirmed, this, &TrackViewItem::LineEditConfirmed);
  connect(line_edit_, &FocusableLineEdit::Cancelled, this, &TrackViewItem::LineEditCancelled);
  stack_->addWidget(line_edit_);

  mute_button_ = CreateMSLButton(tr("M"), Qt::red);
  connect(mute_button_, &QPushButton::toggled, track_, &Track::SetMuted);
  layout->addWidget(mute_button_);

  /*solo_button_ = CreateMSLButton(tr("S"), Qt::yellow);
  layout->addWidget(solo_button_);*/

  lock_button_ = CreateMSLButton(tr("L"), Qt::gray);
  connect(lock_button_, &QPushButton::toggled, track_, &Track::SetLocked);
  layout->addWidget(lock_button_);

  setMinimumHeight(mute_button_->height());

  connect(track, &Track::MutedChanged, mute_button_, &QPushButton::setChecked);
}

QPushButton *TrackViewItem::CreateMSLButton(const QString& text, const QColor& checked_color) const
{
  QPushButton* button = new QPushButton(text);
  button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  button->setCheckable(true);
  button->setStyleSheet(QStringLiteral("QPushButton::checked { background: %1; }").arg(checked_color.name()));

  int size = button->sizeHint().height();
  size = qRound(size * 0.75);
  button->setFixedSize(size, size);

  return button;
}

void TrackViewItem::LabelClicked()
{
  stack_->setCurrentWidget(line_edit_);
  line_edit_->setFocus();
  line_edit_->selectAll();
}

void TrackViewItem::LineEditConfirmed()
{
  line_edit_->blockSignals(true);

  track_->SetLabel(line_edit_->text());
  UpdateLabel();

  stack_->setCurrentWidget(label_);

  line_edit_->blockSignals(false);
}

void TrackViewItem::LineEditCancelled()
{
  line_edit_->blockSignals(true);

  stack_->setCurrentWidget(label_);

  line_edit_->blockSignals(false);
}

void TrackViewItem::UpdateLabel()
{
  if (track_->GetLabel().isEmpty()) {
    label_->setText(track_->GetDefaultTrackName(track_->type(), track_->Index()));
  } else {
    label_->setText(track_->GetLabel());
  }
}

}

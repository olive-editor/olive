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

#include "sliderbase.h"

#include <QDebug>
#include <QEvent>
#include <QMessageBox>

SliderBase::SliderBase(Mode mode, QWidget *parent) :
  QStackedWidget(parent),
  decimal_places_(1),
  drag_multiplier_(1.0),
  mode_(mode),
  dragged_(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  label_ = new SliderLabel(this);
  addWidget(label_);

  editor_ = new SliderLineEdit(this);
  addWidget(editor_);

  connect(label_, SIGNAL(drag_start()), this, SLOT(LabelPressed()));
  connect(label_, SIGNAL(dragged(int)), this, SLOT(LabelDragged(int)));
  connect(label_, SIGNAL(drag_stop()), this, SLOT(LabelClicked()));
  connect(editor_, SIGNAL(Confirmed()), this, SLOT(LineEditConfirmed()));
  connect(editor_, SIGNAL(Cancelled()), this, SLOT(LineEditCancelled()));

  // Set valid cursor based on mode
  switch (mode_) {
  case kString:
    setCursor(Qt::PointingHandCursor);
    value_ = "";
    break;
  case kInteger:
  case kFloat:
    setCursor(Qt::SizeHorCursor);
    value_ = 0;
    break;
  }

  UpdateLabel(value_);
}

void SliderBase::SetDragMultiplier(const double &d)
{
  drag_multiplier_ = d;
}

const QVariant &SliderBase::Value()
{
  return value_;
}

void SliderBase::SetValue(const QVariant &v)
{
  value_ = v;

  UpdateLabel(value_);
}

void SliderBase::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    UpdateLabel(value_);
  }
  QStackedWidget::changeEvent(e);
}

void SliderBase::UpdateLabel(const QVariant &v)
{
  switch (mode_) {
  case kString:
  {
    QString vstr = v.toString();

    if (vstr.isEmpty()) {
      label_->setText(tr("(none)"));
    } else {
      label_->setText(vstr);
    }
    break;
  }
  case kInteger:
    label_->setText(v.toString());
    break;
  case kFloat:
    // For floats, we show a limited number of decimal places
    label_->setText(QString::number(v.toDouble(), 'f', decimal_places_));
    break;
  }
}

void SliderBase::LabelPressed()
{
  dragged_ = false;

  if (mode_ != kString) {
    temp_drag_value_ = value_.toDouble();
  }
}

void SliderBase::LabelClicked()
{
  if (dragged_) {
    // This was a drag
    switch (mode_) {
    case kString:
      // No-op
      break;
    case kInteger:
      value_ = qRound(temp_drag_value_);
      break;
    case kFloat:
      value_ = temp_drag_value_;
      break;
    }

    UpdateLabel(value_);
  } else {
    // This was a simple click

    // Load label's text into editor
    editor_->setText(label_->text());

    // Show editor
    setCurrentWidget(editor_);

    // Select all text in the editor
    editor_->setFocus();
    editor_->selectAll();
  }
}

void SliderBase::LabelDragged(int i)
{
  switch (mode_) {
  case kString:
    // No dragging supported for strings
    break;
  case kInteger:
  case kFloat:
  {
    double real_drag = static_cast<double>(i) * drag_multiplier_;

    temp_drag_value_ += real_drag;

    // Update temporary value for
    QVariant temp_val;

    if (mode_ == kInteger) {
      temp_val = qRound(temp_drag_value_);
    } else {
      temp_val = temp_drag_value_;
    }

    UpdateLabel(temp_val);
    emit ValueChanged(temp_val);
    break;
  }
  }

  dragged_ = true;
}

void SliderBase::LineEditConfirmed()
{
  bool is_valid = true;
  QVariant test_val;

  // Check whether the entered value is valid for this mode
  switch (mode_) {
  case kString:
    // Anything goes for a string
    test_val = editor_->text();
    break;
  case kInteger:
  case kFloat:
    // Allow both floats and integers for either modes
    test_val = editor_->text().toDouble(&is_valid);

    if (is_valid && mode_ == kInteger) {
      // But for an integer, we round it
      test_val = qRound(test_val.toDouble());
    }
    break;
  }

  if (is_valid) {
    value_ = test_val;

    UpdateLabel(value_);

    emit ValueChanged(value_);

    setCurrentWidget(label_);
  } else {
    // Ensure the editor focusing out when the messagebox appears does not cause another messagebox
    editor_->blockSignals(true);

    QMessageBox::critical(this,
                          tr("Invalid Value"),
                          tr("The entered value is not valid for this field."),
                          QMessageBox::Ok);

    // Refocus editor
    editor_->setFocus();

    editor_->blockSignals(false);
  }
}

void SliderBase::LineEditCancelled()
{
  // Ensure editor doesn't signal that the focus is lost
  editor_->blockSignals(true);

  // Set widget back to label
  setCurrentWidget(label_);

  editor_->blockSignals(false);
}

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

OLIVE_NAMESPACE_ENTER

SliderBase::SliderBase(Mode mode, QWidget *parent) :
  QStackedWidget(parent),
  drag_multiplier_(1.0),
  has_min_(false),
  has_max_(false),
  mode_(mode),
  dragged_(false),
  require_valid_input_(true),
  tristate_(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  label_ = new SliderLabel(this);

  addWidget(label_);

  editor_ = new FocusableLineEdit(this);
  addWidget(editor_);

  connect(label_, SIGNAL(drag_start()), this, SLOT(LabelPressed()));
  connect(label_, SIGNAL(dragged(int)), this, SLOT(LabelDragged(int)));
  connect(label_, SIGNAL(drag_stop()), this, SLOT(LabelClicked()));
  connect(label_, SIGNAL(focused()), this, SLOT(LabelClicked()));
  connect(editor_, SIGNAL(Confirmed()), this, SLOT(LineEditConfirmed()));
  connect(editor_, SIGNAL(Cancelled()), this, SLOT(LineEditCancelled()));

  // Set valid cursor based on mode
  switch (mode_) {
  case kString:
    setCursor(Qt::PointingHandCursor);
    SetValue("");
    break;
  case kInteger:
  case kFloat:
    setCursor(Qt::SizeHorCursor);
    SetValue(0);
    break;
  }
}

void SliderBase::SetDragMultiplier(const double &d)
{
  drag_multiplier_ = d;
}

void SliderBase::SetRequireValidInput(bool e)
{
  require_valid_input_ = e;
}

void SliderBase::SetAlignment(Qt::Alignment alignment)
{
  label_->setAlignment(alignment);
}

bool SliderBase::IsTristate() const
{
  return tristate_;
}

void SliderBase::SetTristate()
{
  tristate_ = true;
  UpdateLabel(0);
}

bool SliderBase::IsDragging() const
{
  return dragged_;
}

void SliderBase::SetFormat(const QString &s)
{
  custom_format_ = s;
  ForceLabelUpdate();
}

void SliderBase::ClearFormat()
{
  custom_format_.clear();
  ForceLabelUpdate();
}

void SliderBase::ForceLabelUpdate()
{
  UpdateLabel(Value());
}

const QVariant &SliderBase::Value()
{
  if (dragged_) {
    return temp_dragged_value_;
  }

  return value_;
}

void SliderBase::SetValue(const QVariant &v)
{
  if (IsDragging()) {
    return;
  }

  value_ = ClampValue(v);

  // Disable tristate
  tristate_ = false;

  UpdateLabel(value_);
}

void SliderBase::SetMinimumInternal(const QVariant &v)
{
  min_value_ = v;
  has_min_ = true;

  // Limit value by this new minimum value
  if (value_ < min_value_) {
    SetValue(min_value_);
  }
}

void SliderBase::SetMaximumInternal(const QVariant &v)
{
  max_value_ = v;
  has_max_ = true;

  // Limit value by this new maximum value
  if (value_ > max_value_) {
    SetValue(max_value_);
  }
}

void SliderBase::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    UpdateLabel(value_);
  }
  QStackedWidget::changeEvent(e);
}

const QVariant &SliderBase::ClampValue(const QVariant &v)
{
  if (has_min_ && v < min_value_) {
    return min_value_;
  }

  if (has_max_ && v > max_value_) {
    return max_value_;
  }

  return v;
}

QString SliderBase::GetFormat() const
{
  if (custom_format_.isEmpty()) {
    return QStringLiteral("%1");
  } else {
    return custom_format_;
  }
}

void SliderBase::UpdateLabel(const QVariant &v)
{
  if (tristate_) {
    label_->setText("---");
  } else {
    label_->setText(GetFormat().arg(ValueToString(v)));
  }
}

double SliderBase::AdjustDragDistanceInternal(const double &start, const double &drag)
{
  return start + drag;
}

QString SliderBase::ValueToString(const QVariant &v)
{
  return v.toString();
}

QVariant SliderBase::StringToValue(const QString &s, bool *ok)
{
  *ok = true;
  return s;
}

void SliderBase::LabelPressed()
{
  dragged_ = false;
  dragged_diff_ = 0;
}

void SliderBase::LabelClicked()
{
  if (dragged_) {
    dragged_ = false;

    // This was a drag
    switch (mode_) {
    case kString:
      // No-op
      break;
    case kInteger:
      SetValue(temp_dragged_value_.toInt());
      break;
    case kFloat:
      SetValue(temp_dragged_value_.toDouble());
      break;
    }

    emit ValueChanged(value_);
  } else {
    // This was a simple click

    // Load label's text into editor
    editor_->setText(ValueToString(value_));

    // Show editor
    setCurrentWidget(editor_);

    // Select all text in the editor
    editor_->setFocus();
    editor_->selectAll();
  }
}

void SliderBase::LabelDragged(int i)
{
  dragged_ = true;

  switch (mode_) {
  case kString:
    // No dragging supported for strings
    break;
  case kInteger:
  case kFloat:
  {
    dragged_diff_ += static_cast<double>(i) * drag_multiplier_;

    double drag_val = AdjustDragDistanceInternal(value_.toDouble(), dragged_diff_);

    // Update temporary value
    if (mode_ == kInteger) {
      temp_dragged_value_ = qRound(drag_val);
    } else {
      temp_dragged_value_ = drag_val;
    }

    temp_dragged_value_ = ClampValue(temp_dragged_value_);

    UpdateLabel(temp_dragged_value_);
    emit ValueChanged(temp_dragged_value_);
    break;
  }
  }
}

void SliderBase::LineEditConfirmed()
{
  bool is_valid = true;
  QVariant test_val = StringToValue(editor_->text(), &is_valid);

  // Ensure editor doesn't signal that the focus is lost
  editor_->blockSignals(true);
  label_->blockSignals(true);

  if (is_valid) {
    SetValue(test_val);

    setCurrentWidget(label_);

    emit ValueChanged(value_);
  } else if (require_valid_input_ && !IsTristate()) {
    QMessageBox::critical(this,
                          tr("Invalid Value"),
                          tr("The entered value is not valid for this field."),
                          QMessageBox::Ok);

    // Refocus editor
    editor_->setFocus();
  }

  editor_->blockSignals(false);
  label_->blockSignals(false);
}

void SliderBase::LineEditCancelled()
{
  // Ensure editor doesn't signal that the focus is lost
  editor_->blockSignals(true);
  label_->blockSignals(true);

  // Set widget back to label
  setCurrentWidget(label_);

  editor_->blockSignals(false);
  label_->blockSignals(false);
}

OLIVE_NAMESPACE_EXIT

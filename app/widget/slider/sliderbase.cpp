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

#include "sliderbase.h"

#include <QDebug>
#include <QEvent>
#include <QMessageBox>

#include "common/qtutils.h"
#include "core.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

SliderBase::SliderBase(Mode mode, QWidget *parent) :
  QStackedWidget(parent),
  drag_multiplier_(1.0),
  has_min_(false),
  has_max_(false),
  mode_(mode),
  dragged_diff_(0),
  require_valid_input_(true),
  tristate_(false),
  drag_ladder_(nullptr),
  ladder_element_count_(0),
  dragged_(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  label_ = new SliderLabel(this);

  addWidget(label_);

  editor_ = new FocusableLineEdit(this);
  addWidget(editor_);

  connect(label_, &SliderLabel::LabelPressed, this, &SliderBase::LabelPressed);
  connect(label_, &SliderLabel::focused, this, &SliderBase::ShowEditor);
  connect(label_, &SliderLabel::RequestReset, this, &SliderBase::ResetValue);
  connect(editor_, &FocusableLineEdit::Confirmed, this, &SliderBase::LineEditConfirmed);
  connect(editor_, &FocusableLineEdit::Cancelled, this, &SliderBase::LineEditCancelled);

  // Set valid cursor based on mode
  switch (mode_) {
  case kString:
    setCursor(Qt::PointingHandCursor);
    SetValue(QString());
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
  return drag_ladder_;
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

const QVariant &SliderBase::Value() const
{
  if (IsDragging()) {
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

void SliderBase::SetDefaultValue(const QVariant &v)
{
  default_value_ = v;
}

void SliderBase::SetOffset(const QVariant &v)
{
  offset_ = v;

  UpdateLabel(value_);
}

void SliderBase::SetMinimumInternal(const QVariant &v)
{
  min_value_ = v;
  has_min_ = true;

  // Limit value by this new minimum value
  if (value_.toDouble() < min_value_.toDouble()) {
    SetValue(min_value_);
  }
}

void SliderBase::SetMaximumInternal(const QVariant &v)
{
  max_value_ = v;
  has_max_ = true;

  // Limit value by this new maximum value
  if (value_.toDouble() > max_value_.toDouble()) {
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
  if (has_min_ && v.toDouble() < min_value_.toDouble()) {
    return min_value_;
  }

  if (has_max_ && v.toDouble() > max_value_.toDouble()) {
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

void SliderBase::ShowEditor()
{
  // This was a simple click
  // Load label's text into editor
  editor_->setText(ValueToString(value_));

  // Show editor
  setCurrentWidget(editor_);

  // Select all text in the editor
  editor_->setFocus();
  editor_->selectAll();
}

void SliderBase::LabelPressed()
{
  switch (mode_) {
  case kString:
    // No dragging supported for strings
    break;
  case kInteger:
  case kFloat:
  {
    drag_ladder_ = new SliderLadder(drag_multiplier_, ladder_element_count_);
    drag_ladder_->SetValue(ValueToString(value_));
    drag_ladder_->show();

    QMetaObject::invokeMethod(this, "RepositionLadder", Qt::QueuedConnection);

    connect(drag_ladder_, &SliderLadder::DraggedByValue, this, &SliderBase::LadderDragged);
    connect(drag_ladder_, &SliderLadder::Released, this, &SliderBase::LadderReleased);
    break;
  }
  }
}

void SliderBase::LadderDragged(int value, double multiplier)
{
  dragged_ = true;

  switch (mode_) {
  case kString:
    // No dragging supported for strings
    break;
  case kInteger:
  case kFloat:
  {
    dragged_diff_ += value * drag_multiplier_ * multiplier;

    double drag_val = AdjustDragDistanceInternal(value_.toDouble(), dragged_diff_);

    // Update temporary value
    if (mode_ == kInteger) {
      temp_dragged_value_ = qRound(drag_val);
    } else {
      temp_dragged_value_ = drag_val;
    }

    QVariant clamped = ClampValue(temp_dragged_value_);

    if (clamped != temp_dragged_value_) {
      temp_dragged_value_ = clamped;
      dragged_diff_ = temp_dragged_value_.toDouble() - value_.toDouble();
    }

    UpdateLabel(temp_dragged_value_);

    drag_ladder_->SetValue(ValueToString(temp_dragged_value_));

    if (!Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {
      RepositionLadder();
    }

    emit ValueChanged(temp_dragged_value_);
    break;
  }
  }
}

void SliderBase::LadderReleased()
{
  drag_ladder_->deleteLater();
  drag_ladder_ = nullptr;
  dragged_diff_ = 0;

  if (dragged_) {
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

    dragged_ = false;
  } else {
    ShowEditor();
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

void SliderBase::ResetValue()
{
  if (!default_value_.isNull()) {
    SetValue(default_value_);
    emit ValueChanged(value_);
  }
}

void SliderBase::RepositionLadder()
{
  if (Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {
    drag_ladder_->move(QCursor::pos() - QPoint(drag_ladder_->width()/2, drag_ladder_->height()/2));
  } else {
    QPoint label_global_pos = label_->mapToGlobal(label_->pos());
    int text_width = QtUtils::QFontMetricsWidth(label_->fontMetrics(), label_->text());

    int ladder_x = label_global_pos.x() + text_width / 2 - drag_ladder_->width() / 2;
    int ladder_y = label_global_pos.y() + label_->height() / 2 - drag_ladder_->height() / 2;

    drag_ladder_->move(ladder_x, ladder_y);
  }
}

}

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

#include "sliderladder.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QtMath>
#include <QVBoxLayout>

#include "common/clamp.h"

OLIVE_NAMESPACE_ENTER

SliderLadder::SliderLadder(double start_val, double drag_multiplier, int nb_outer_values, QWidget* parent) :
  QFrame(parent, Qt::Popup),
  start_val_(start_val),
  active_element_(nullptr),
  relative_y_(-1)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  setFrameShape(QFrame::Box);
  setLineWidth(1);

  for (int i=nb_outer_values-1;i>=0;i--) {
    elements_.append(new SliderLadderElement(qPow(10, i + 1) * drag_multiplier));
  }

  elements_.append(new SliderLadderElement(drag_multiplier));

  for (int i=0;i<nb_outer_values;i++) {
    elements_.append(new SliderLadderElement(qPow(10, - i - 1) * drag_multiplier));
  }

  foreach (SliderLadderElement* e, elements_) {
    e->SetValue(start_val_);
    layout->addWidget(e);
  }

  if (elements_.size() == 1) {
    elements_.first()->SetMultiplierVisible(false);
  }

#if defined(Q_OS_MAC)
  CGAssociateMouseAndMouseCursorPosition(false);
  CGDisplayHideCursor(kCGDirectMainDisplay);
  CGGetLastMouseDelta(nullptr, nullptr);
#else
  drag_start_ = QCursor::pos();

  static_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::BlankCursor);
#endif
}

SliderLadder::~SliderLadder()
{
#if defined(Q_OS_MAC)
  CGAssociateMouseAndMouseCursorPosition(true);
  CGDisplayShowCursor(kCGDirectMainDisplay);
#else
  static_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();
#endif
}

void SliderLadder::SetValue(double val)
{
  foreach (SliderLadderElement* e, elements_) {
    e->SetValue(val);
  }
}

void SliderLadder::mouseMoveEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  int32_t x_mvmt, y_mvmt;

  // Keep cursor in the same position
#if defined(Q_OS_MAC)
  CGGetLastMouseDelta(&x_mvmt, &y_mvmt);
#else
  QPoint current_pos = QCursor::pos();

  x_mvmt = current_pos.x() - drag_start_.x();
  y_mvmt = current_pos.y() - drag_start_.y();

  QCursor::setPos(drag_start_);
#endif

  // Determine which element we're in
  relative_y_ = clamp(relative_y_ + y_mvmt,
                      elements_.first()->y(),
                      elements_.last()->y() + elements_.last()->height() - 1);

  SetActiveElement();

  emit DraggedByValue(x_mvmt, active_element_->GetMultiplier());
}

void SliderLadder::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  emit Released();
}

void SliderLadder::showEvent(QShowEvent *event)
{
  QWidget::showEvent(event);

  QMetaObject::invokeMethod(this, "InitRelativeY", Qt::QueuedConnection);
}

void SliderLadder::SetActiveElement()
{
  if (!active_element_
      || relative_y_ < active_element_->y()
      || relative_y_ >= active_element_->y() + active_element_->height()) {
    if (active_element_) {
      // Un-highlight active element if one is set
      active_element_->SetHighlighted(false);
    }

    // Find new active element
    foreach (SliderLadderElement* ele, elements_) {
      if (relative_y_ >= ele->y() && relative_y_ < ele->y() + ele->height()) {
        // This is the element!
        active_element_ = ele;
        active_element_->SetHighlighted(true);
        break;
      }
    }
  }
}

void SliderLadder::InitRelativeY()
{
  relative_y_ = QCursor::pos().y() - this->y();

  SetActiveElement();
}

SliderLadderElement::SliderLadderElement(const double &multiplier, QWidget *parent) :
  QWidget(parent),
  multiplier_(multiplier),
  highlighted_(false),
  multiplier_visible_(true)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  label_ = new QLabel();
  label_->setAlignment(Qt::AlignCenter);
  layout->addWidget(label_);

  QPalette p = palette();
  QColor highlight_color = palette().text().color();
  highlight_color.setAlpha(64);
  p.setColor(QPalette::Highlight, highlight_color);
  setPalette(p);

  setAutoFillBackground(true);

  UpdateLabel();
}

void SliderLadderElement::SetHighlighted(bool e)
{
  highlighted_ = e;

  if (highlighted_) {
    setBackgroundRole(QPalette::Highlight);
  } else {
    setBackgroundRole(QPalette::Window);
  }

  UpdateLabel();
}

void SliderLadderElement::SetValue(double value)
{
  value_ = value;

  UpdateLabel();
}

void SliderLadderElement::SetMultiplierVisible(bool e)
{
  multiplier_visible_ = e;

  UpdateLabel();
}

void SliderLadderElement::UpdateLabel()
{
  if (multiplier_visible_) {
    QString val_text;

    if (highlighted_) {
      val_text = QString::number(value_);
    }

    label_->setText(QStringLiteral("%1\n%2").arg(QString::number(multiplier_),
                                                 val_text));
  } else {
    label_->setText(QString::number(value_));
  }
}

OLIVE_NAMESPACE_EXIT

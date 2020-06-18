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

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "common/clamp.h"
#include "common/lerp.h"

OLIVE_NAMESPACE_ENTER

SliderLadder::SliderLadder(double drag_multiplier, int nb_outer_values, QWidget* parent) :
  QFrame(parent, Qt::Popup),
  y_mobility_(0)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  setFrameShape(QFrame::Box);
  setLineWidth(1);

  for (int i=nb_outer_values-1;i>=0;i--) {
    elements_.append(new SliderLadderElement(qPow(10, i + 1) * drag_multiplier));
  }

  // Create center entry
  SliderLadderElement* start_element = new SliderLadderElement(drag_multiplier);
  active_element_ = elements_.size();
  start_element->SetHighlighted(true);
  elements_.append(start_element);

  for (int i=0;i<nb_outer_values;i++) {
    elements_.append(new SliderLadderElement(qPow(10, - i - 1) * drag_multiplier));
  }

  foreach (SliderLadderElement* e, elements_) {
    layout->addWidget(e);
  }

  if (elements_.size() == 1) {
    elements_.first()->SetMultiplierVisible(false);
  }

  drag_timer_.setInterval(10);
  connect(&drag_timer_, &QTimer::timeout, this, &SliderLadder::TimerUpdate);

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

void SliderLadder::SetValue(const QString &s)
{
  foreach (SliderLadderElement* e, elements_) {
    e->SetValue(s);
  }
}

void SliderLadder::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  emit Released();
}

void SliderLadder::showEvent(QShowEvent *event)
{
  QWidget::showEvent(event);

  drag_timer_.start();
}

void SliderLadder::TimerUpdate()
{
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

  int y_threshold = fontMetrics().height() / 2;

  if (qAbs(y_mvmt) > qAbs(x_mvmt)
      || qApp->keyboardModifiers() & Qt::ControlModifier) {
    // Movement is vertical
    y_mobility_ += y_mvmt;

    if (qAbs(y_mobility_) > y_threshold) {
      int new_active_element;

      if (y_mvmt < 0) {
        // Movement is UP
        new_active_element = active_element_ - 1;
      } else {
        // Movement is DOWN
        new_active_element = active_element_ + 1;
      }

      // Check if the proposed element is valid
      if (new_active_element >= 0 && new_active_element < elements_.size()) {
        elements_.at(active_element_)->SetHighlighted(false);

        active_element_ = new_active_element;

        elements_.at(active_element_)->SetHighlighted(true);
      }

      y_mobility_ = 0;
    }
  } else {
    // Movement is horizontal
    emit DraggedByValue(x_mvmt , elements_.at(active_element_)->GetMultiplier());

    // Reduce Y mobility
    if (y_mobility_ > 0) {
      y_mobility_--;
    } else if (y_mobility_ < 0) {
      y_mobility_++;
    }
  }
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

void SliderLadderElement::SetValue(const QString &value)
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
      val_text = value_;
    }

    label_->setText(QStringLiteral("%1\n%2").arg(QString::number(multiplier_),
                                                 val_text));
  } else {
    label_->setText(value_);
  }
}

OLIVE_NAMESPACE_EXIT

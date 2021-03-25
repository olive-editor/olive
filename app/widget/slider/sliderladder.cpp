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
#include "common/qtutils.h"
#include "config/config.h"

namespace olive {

SliderLadder::SliderLadder(double drag_multiplier, int nb_outer_values, QWidget* parent) :
  QFrame(parent, Qt::Popup)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  setFrameShape(QFrame::Box);
  setLineWidth(1);

  if (!Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {
    nb_outer_values = 0;
  }

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

  if (Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {
    drag_start_x_ = -1;
  } else {
#if defined(Q_OS_MAC)
    CGAssociateMouseAndMouseCursorPosition(false);
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGGetLastMouseDelta(nullptr, nullptr);
#else
    drag_start_x_ = QCursor::pos().x();
    drag_start_y_ = QCursor::pos().y();

    static_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::BlankCursor);
#endif
  }
}

SliderLadder::~SliderLadder()
{
  if (Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {

  } else {
#if defined(Q_OS_MAC)
    CGAssociateMouseAndMouseCursorPosition(true);
    CGDisplayShowCursor(kCGDirectMainDisplay);
#else
    static_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();
#endif
  }
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

  this->close();
}

void SliderLadder::showEvent(QShowEvent *event)
{
  QWidget::showEvent(event);

  drag_timer_.start();
}

void SliderLadder::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event)

  drag_timer_.stop();

  emit Released();
}

void SliderLadder::TimerUpdate()
{
  int ladder_left = this->x();
  int ladder_right = this->x() + this->width() - 1;
  int now_pos = QCursor::pos().x();

  if (Config::Current()[QStringLiteral("UseSliderLadders")].toBool()) {

    bool is_under_mouse = (now_pos >= ladder_left && now_pos <= ladder_right);

    if (drag_start_x_ != -1 && (is_under_mouse
        || (drag_start_x_ < ladder_left && now_pos > ladder_right)
        || (drag_start_x_ > ladder_right && now_pos < ladder_left))) {
      // We're ending a drag, try to return the value back to its beginning
      int anchor;

      if (drag_start_x_ < ladder_left) {
        anchor = ladder_left;
      } else {
        anchor = ladder_right;
      }

      int makeup_value = anchor - drag_start_x_;
      emit DraggedByValue(makeup_value, elements_.at(active_element_)->GetMultiplier());

      drag_start_x_ = -1;
    }

    if (is_under_mouse) {

      // Determine which element is currently active
      for (int i=0; i<elements_.size(); i++) {
        if (elements_.at(i)->underMouse()) {
          if (i != active_element_) {
            elements_.at(active_element_)->SetHighlighted(false);
            active_element_ = i;
            elements_.at(active_element_)->SetHighlighted(true);
          }

          break;
        }
      }

    } else {

      if (drag_start_x_ == -1) {
        // Drag is a new leave from the ladder, calculate origin
        if (now_pos < ladder_left) {
          drag_start_x_ = ladder_left;
        } else {
          drag_start_x_ = ladder_right;
        }
      }

      emit DraggedByValue(now_pos - drag_start_x_, elements_.at(active_element_)->GetMultiplier());
      drag_start_x_ = now_pos;

    }

  } else {
    int32_t x_mvmt, y_mvmt;

    // Keep cursor in the same position
#if defined(Q_OS_MAC)
    CGGetLastMouseDelta(&x_mvmt, &y_mvmt);
#else
    QPoint current_pos = QCursor::pos();

    x_mvmt = current_pos.x() - drag_start_x_;
    y_mvmt = current_pos.y() - drag_start_y_;

    QCursor::setPos(QPoint(drag_start_x_, drag_start_y_));
#endif

    if (x_mvmt || y_mvmt) {
      double multiplier = 1.0;

      if (qApp->keyboardModifiers() & Qt::ControlModifier) {
        multiplier *= 0.1;
      }

      if (qApp->keyboardModifiers() & Qt::ShiftModifier) {
        multiplier *= 0.1;
      }

      emit DraggedByValue(x_mvmt + y_mvmt, multiplier);
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
  label_->setFixedWidth(QtUtils::QFontMetricsWidth(label_->fontMetrics(), QStringLiteral("0000000")));
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

}

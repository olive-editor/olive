/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "labelslider.h"

#include "project/undo.h"
#include "panels/viewer.h"
#include "io/config.h"
#include "io/math.h"
#include "debug.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QApplication>
#include <QMenu>

LabelSlider::LabelSlider(QWidget* parent) : QLabel(parent) {
  // set a default frame rate - fallback, shouldn't ever really be used
  frame_rate = 30;

  decimal_places = 1;
  drag_start = false;
  drag_proc = false;
  min_enabled = false;
  max_enabled = false;
  set_color();
  set_default_cursor();
  internal_value = -1;
  set = false;
  display_type = LABELSLIDER_NORMAL;

  set_default_value(0);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void LabelSlider::set_frame_rate(double d) {
  frame_rate = d;
}

void LabelSlider::set_display_type(const DisplayType& type) {
  display_type = type;
  setText(valueToString());
}

void LabelSlider::set_value(double v, bool userSet) {
  set = true;
  if (!qFuzzyCompare(v, internal_value)) {
    if (min_enabled && v < min_value) {
      internal_value = min_value;
    } else if (max_enabled && v > max_value) {
      internal_value = max_value;
    } else {
      internal_value = v;
    }

    setText(valueToString());
    if (userSet) {
      emit valueChanged(internal_value);
    }
  }
}

bool LabelSlider::is_set() {
  return set;
}

bool LabelSlider::is_dragging() {
  return drag_proc;
}

QString LabelSlider::valueToString() {
  double v = internal_value;
  if (qIsNaN(v)) {
    return "---";
  } else {
    switch (display_type) {
    case LABELSLIDER_FRAMENUMBER:
      return frame_to_timecode(long(v), olive::CurrentConfig.timecode_view, frame_rate);
    case LABELSLIDER_PERCENT:
      return QString::number((v*100), 'f', decimal_places).append("%");
    case LABELSLIDER_DECIBEL:
    {
      QString db_str;

      // -96 dB is considered -infinity
      if (amplitude_to_db(v) <= -96) {
        // hex sequence for -infinity
        db_str = "-\xE2\x88\x9E";
      } else {
        db_str = QString::number(amplitude_to_db(v), 'f', decimal_places);
      }

      // add "dB" suffix
      db_str.append(" dB");

      return db_str;
    }
    default:
      return QString::number(v, 'f', decimal_places);
    }
  }
}

double LabelSlider::getPreviousValue() {
  return previous_value;
}

void LabelSlider::set_previous_value() {
  previous_value = internal_value;
}

void LabelSlider::set_color(QString c) {
  if (c.isEmpty()) c = "#ffc000";
  setStyleSheet("QLabel{color:" + c + ";text-decoration:underline;}QLabel:disabled{color:#808080;}");
}

double LabelSlider::value() {
  return internal_value;
}

void LabelSlider::set_default_value(double v) {
  default_value = v;
  if (!set) {
    set_value(v, false);
    set = false;
  }
}

void LabelSlider::set_minimum_value(double v) {
  min_value = v;
  min_enabled = true;
}

void LabelSlider::set_maximum_value(double v) {
  max_value = v;
  max_enabled = true;
}

void LabelSlider::mousePressEvent(QMouseEvent *ev) {
  // if primary button is clicked
  if (ev->button() == Qt::LeftButton && !drag_start) {

    // store initial value to be compared while dragging
    drag_start_value = internal_value;

    // alt + click sets labelslider to default value
    if (ev->modifiers() & Qt::AltModifier) {

      // reset to default
      reset_to_default_value();

    } else {

      // if value isn't valid, we set to a "default" of 0
      if (qIsNaN(internal_value)) internal_value = 0;

      // hide the cursor and store information about it for dragging
      set_active_cursor();

      drag_start = true;
      drag_start_x = cursor().pos().x();
      drag_start_y = cursor().pos().y();
    }

    emit clicked();

  } else {

    // handler if the user clicks with a non-primary button
    // prevents mouse from getting stuck in "dragging" mode
    mouseReleaseEvent(ev);

  }
}

void LabelSlider::mouseMoveEvent(QMouseEvent* event) {
  if (drag_start) {
    // if we're dragging

    // we don't actually start fully dragging until the cursor has moved
    // while the mouse is down, this helps prevent accidental drags
    drag_proc = true;

    // get amount cursor moved by
    double diff = (cursor().pos().x()-drag_start_x) + (drag_start_y-cursor().pos().y());

    // ctrl + drag drags in smaller increments
    if (event->modifiers() & Qt::ControlModifier) diff *= 0.01;

    if (display_type == LABELSLIDER_PERCENT) {
      // we'll also need to drag in smaller increments for a percent value

      diff *= 0.01;
    }

    // determine what the new value will be
    double new_value;

    if (display_type == LABELSLIDER_DECIBEL) {
      // we move in terms of dB for decibel display

      new_value = db_to_amplitude(amplitude_to_db(internal_value) + diff);

    } else {
      // for most display types, just add the mouse difference

      new_value = internal_value + diff;
    }

    // set internal value
    set_value(new_value, true);

    // keep the cursor in the same location while dragging
    cursor().setPos(drag_start_x, drag_start_y);
  }
}

void LabelSlider::mouseReleaseEvent(QMouseEvent*) {
  if (drag_start) {

    // unhide cursor
    set_default_cursor();

    drag_start = false;

    if (drag_proc) {
      // if we just finished fully dragging

      drag_proc = false;

      previous_value = drag_start_value;

      emit valueChanged(internal_value);

    } else {

      prompt_for_value();

    }
  }
}

void LabelSlider::set_default_cursor() {
  setCursor(Qt::SizeHorCursor);
}

void LabelSlider::set_active_cursor() {
  setCursor(Qt::BlankCursor);
}

void LabelSlider::show_context_menu(const QPoint &pos)
{
  QMenu menu(this);

  menu.addAction(tr("&Edit"), this, SLOT(prompt_for_value()));

  menu.addSeparator();

  menu.addAction(tr("&Reset to Default"), this, SLOT(reset_to_default_value()));

  menu.exec(mapToGlobal(pos));
}

void LabelSlider::reset_to_default_value()
{
  // if the value is not already default, and there is a default to set
  if (!qFuzzyCompare(internal_value, default_value)
      && !qIsNaN(default_value)) {

    // cache current value
    set_previous_value();

    // set back to default
    set_value(default_value, true);

  }
}

void LabelSlider::prompt_for_value()
{
  // if the user didn't actually drag, and just clicked in one place,
  // we instead present a dialog prompt for the user to enter a
  // specific value

  double d = internal_value;

  if (display_type == LABELSLIDER_FRAMENUMBER) {

    // ask the user to enter a timecode
    QString s = QInputDialog::getText(
          this,
          tr("Set Value"),
          tr("New value:"),
          QLineEdit::Normal,
          valueToString()
          );
    if (s.isEmpty()) return;

    // parse string timecode to a frame number
    d = timecode_to_frame(s, olive::CurrentConfig.timecode_view, frame_rate);

  } else {

    // ask the user to enter a normal number value
    bool ok;

    // value to show
    double shown_value = internal_value;
    if (display_type == LABELSLIDER_PERCENT) {
      shown_value *= 100;
    } else if (display_type == LABELSLIDER_DECIBEL) {
      shown_value = amplitude_to_db(shown_value);
    }

    // set correct minimum value
    double shown_minimum_value;
    if (min_enabled) {
      // if this field has a minimum value set, use it
      shown_minimum_value = min_value;
    } else if (display_type == LABELSLIDER_DECIBEL) {
      // minimum decibel amount is -96db
      shown_minimum_value = -96;
    } else {
      // lowest possible minimum integer
      shown_minimum_value = INT_MIN;
    }

    // percentages are stored 0.0 - 1.0 but displayed as 0% - 100%
    d = QInputDialog::getDouble(
          this,
          tr("Set Value"),
          tr("New value:"),
          shown_value,
          shown_minimum_value,
          (max_enabled) ? max_value : INT_MAX,
          decimal_places,
          &ok
          );
    if (!ok) return;

    // convert shown value back to internal value
    if (display_type == LABELSLIDER_PERCENT) {
      d *= 0.01;
    } else if (display_type == LABELSLIDER_DECIBEL) {
      d = db_to_amplitude(d);
    }
  }

  // if the value actually changed, trigger a change event
  if (!qFuzzyCompare(d, internal_value)) {
    set_previous_value();
    set_value(d, true);
  }
}

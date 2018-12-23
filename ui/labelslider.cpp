#include "labelslider.h"

#include "project/undo.h"
#include "panels/viewer.h"
#include "io/config.h"
#include "debug.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QApplication>

LabelSlider::LabelSlider(QWidget* parent) : QLabel(parent) {
	frame_rate = 30;
	decimal_places = 1;
    drag_start = false;
	drag_proc = false;
    min_enabled = false;
    max_enabled = false;
    setStyleSheet("QLabel{color:#ffc000;text-decoration:underline;}QLabel:disabled{color:#808080;}");
    setCursor(Qt::SizeHorCursor);
    internal_value = -1;
    set = false;
    display_type = LABELSLIDER_NORMAL;

    set_default_value(0);
}

void LabelSlider::set_frame_rate(double d) {
	frame_rate = d;
}

void LabelSlider::set_display_type(int type) {
	display_type = type;
	setText(valueToString(internal_value));
}

void LabelSlider::set_value(double v, bool userSet) {
    set = true;
    if (v != internal_value) {
        if (min_enabled && v < min_value) {
            internal_value = min_value;
        } else if (max_enabled && v > max_value) {
            internal_value = max_value;
        } else {
            internal_value = v;
        }

		setText(valueToString(internal_value));
        if (userSet) emit valueChanged();
    }
}

bool LabelSlider::is_set() {
    return set;
}

bool LabelSlider::is_dragging() {
	return drag_proc;
}

QString LabelSlider::valueToString(double v) {
	if (qIsNaN(v)) {
		return "---";
	} else {
		switch (display_type) {
		case LABELSLIDER_FRAMENUMBER: return frame_to_timecode(v, config.timecode_view, frame_rate);
		case LABELSLIDER_PERCENT: return QString::number((v*100), 'f', decimal_places) + "%";
		}
		return QString::number(v, 'f', decimal_places);
	}
}

double LabelSlider::getPreviousValue() {
    return previous_value;
}

void LabelSlider::set_previous_value() {
    previous_value = internal_value;
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
	drag_start_value = internal_value;
	if (ev->modifiers() & Qt::AltModifier) {
		if (internal_value != default_value && !qIsNaN(default_value)) {
            set_previous_value();
			set_value(default_value, true);
		}
    } else {
		if (qIsNaN(internal_value)) internal_value = 0;

        qApp->setOverrideCursor(Qt::BlankCursor);
        drag_start = true;
        drag_start_x = cursor().pos().x();
        drag_start_y = cursor().pos().y();
    }
}

void LabelSlider::mouseMoveEvent(QMouseEvent* event) {
    if (drag_start) {
		drag_proc = true;
        double diff = (cursor().pos().x()-drag_start_x) + (drag_start_y-cursor().pos().y());
        if (event->modifiers() & Qt::ControlModifier) diff *= 0.01;
        if (display_type == LABELSLIDER_PERCENT) diff *= 0.01;
        set_value(internal_value + diff, true);
        cursor().setPos(drag_start_x, drag_start_y);
    }
}

void LabelSlider::mouseReleaseEvent(QMouseEvent*) {
    if (drag_start) {
        qApp->restoreOverrideCursor();
        drag_start = false;
        if (drag_proc) {
			drag_proc = false;
			previous_value = drag_start_value;
            emit valueChanged();
		} else {
			double d = internal_value;
			if (display_type == LABELSLIDER_FRAMENUMBER) {
				QString s = QInputDialog::getText(
							this,
							"Set Value",
							"New value:",
							QLineEdit::Normal,
							valueToString(internal_value)
						);
                if (s.isEmpty()) return;
				d = timecode_to_frame(s, config.timecode_view, frame_rate); // string to frame number
			} else {
                bool ok;
				d = QInputDialog::getDouble(
							this,
							"Set Value",
							"New value:",
							(display_type == LABELSLIDER_PERCENT) ? internal_value * 100 : internal_value,
							(min_enabled) ? min_value : INT_MIN,
							(max_enabled) ? max_value : INT_MAX,
                            decimal_places,
                            &ok
						);
                if (!ok) return;
                if (display_type == LABELSLIDER_PERCENT) d *= 0.01;
			}
			if (d != internal_value) {
                set_previous_value();
				set_value(d, true);
			}
		}
    }
}

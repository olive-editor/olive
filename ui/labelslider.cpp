#include "labelslider.h"

#include "project/undo.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QApplication>

LabelSlider::LabelSlider(QWidget* parent) : QLabel(parent) {
    drag_start = false;
	drag_proc = false;
    min_enabled = false;
    max_enabled = false;
    setStyleSheet("QLabel{color:#ffc000;text-decoration:underline;}QLabel:disabled{color:#808080;}");
    setCursor(Qt::SizeHorCursor);
    internal_value = -1;
    set_default_value(0);
	set_value(0, false);
    set = false;
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
	return QString::number(v, 'f', 1);
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

double LabelSlider::get_drag_start_value() {
	return drag_start_value;
}

void LabelSlider::mousePressEvent(QMouseEvent *ev) {
	drag_start_value = internal_value;
	if (ev->modifiers() & Qt::AltModifier) {
		set_value(default_value, true);
    } else {
        qApp->setOverrideCursor(Qt::BlankCursor);
        drag_start = true;
        drag_start_x = cursor().pos().x();
        drag_start_y = cursor().pos().y();
    }
}

void LabelSlider::mouseMoveEvent(QMouseEvent*) {
    if (drag_start) {
		drag_proc = true;
		set_value(internal_value + (cursor().pos().x()-drag_start_x) + (drag_start_y-cursor().pos().y()), true);
        cursor().setPos(drag_start_x, drag_start_y);
    }
}

void LabelSlider::mouseReleaseEvent(QMouseEvent*) {
    if (drag_start) {
        qApp->restoreOverrideCursor();
        drag_start = false;
        if (drag_proc) {
			drag_proc = false;
			emit valueChanged();
        } else {
            double d = QInputDialog::getDouble(this, "Set Value", "New value:", internal_value);
            if (d != internal_value) {
				set_value(d, true);
            }
        }
    }
}

#include "labelslider.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QApplication>

LabelSlider::LabelSlider(QWidget* parent) : QLabel(parent)
{
    drag_start = false;
    min_enabled = false;
    max_enabled = false;
    setStyleSheet("QLabel{color:#ffc000;text-decoration:underline;}QLabel:disabled{color:#808080;}");
    setCursor(Qt::SizeHorCursor);
    set_value(0);
    set_default_value(0);
}

void LabelSlider::set_value(float v) {
    if (v != internal_value) {
        if (min_enabled && v < min_value) {
            internal_value = min_value;
        } else if (max_enabled && v > max_value) {
            internal_value = max_value;
        } else {
            internal_value = v;
        }
        setText(QString::number(v));
        emit valueChanged();
    }
}

float LabelSlider::value() {
    return internal_value;
}

void LabelSlider::set_default_value(float v) {
    default_value = v;
    set_value(v);
}

void LabelSlider::set_minimum_value(float v) {
    min_value = v;
    min_enabled = true;
}

void LabelSlider::set_maximum_value(float v) {
    max_value = v;
    max_enabled = true;
}

void LabelSlider::mousePressEvent(QMouseEvent *ev) {
    if (ev->modifiers() & Qt::AltModifier) {
        set_value(default_value);
    } else {
        qApp->setOverrideCursor(Qt::BlankCursor);
        drag_start = true;
        drag_start_x = cursor().pos().x();
        drag_start_y = cursor().pos().y();
    }
}

void LabelSlider::mouseMoveEvent(QMouseEvent *ev) {
    if (drag_start) {
        set_value(internal_value + (cursor().pos().x()-drag_start_x) + (drag_start_y-cursor().pos().y()));
        cursor().setPos(drag_start_x, drag_start_y);
        drag_proc = true;
    }
}

void LabelSlider::mouseReleaseEvent(QMouseEvent *ev) {
    if (drag_start) {
        qApp->restoreOverrideCursor();
        drag_start = false;
        if (drag_proc) {
            drag_proc = false;
        } else {
            set_value(QInputDialog::getDouble(this, "Set Value", "New value:", internal_value));
        }
    }
}

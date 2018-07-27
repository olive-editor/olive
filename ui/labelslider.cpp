#include "labelslider.h"

#include "project/undo.h"

#include <QMouseEvent>
#include <QInputDialog>
#include <QApplication>

LabelSlider::LabelSlider(QWidget* parent) : QLabel(parent) {
    drag_start = false;
    min_enabled = false;
    max_enabled = false;
    setStyleSheet("QLabel{color:#ffc000;text-decoration:underline;}QLabel:disabled{color:#808080;}");
    setCursor(Qt::SizeHorCursor);
    internal_value = -1;
    set_default_value(0);
    set_value(0);
    set = false;
}

void LabelSlider::set_value(double v) {
    set = true;
    if (v != internal_value) {
        if (min_enabled && v < min_value) {
            internal_value = min_value;
        } else if (max_enabled && v > max_value) {
            internal_value = max_value;
        } else {
            internal_value = v;
        }

        setText(QString::number(internal_value));
        emit valueChanged();
    }
}

bool LabelSlider::is_set() {
    return set;
}

double LabelSlider::value() {
    return internal_value;
}

void LabelSlider::set_default_value(double v) {
    default_value = v;
    if (!set) set_value(v);
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
    if (ev->modifiers() & Qt::AltModifier) {
        ValueChangeCommand* vcc = new ValueChangeCommand();
        vcc->source = this;
        vcc->old_val = internal_value;
        vcc->new_val = default_value;
        undo_stack.push(vcc);

        set_value(default_value);
    } else {
        qApp->setOverrideCursor(Qt::BlankCursor);
        drag_start_value = internal_value;
        drag_start = true;
        drag_start_x = cursor().pos().x();
        drag_start_y = cursor().pos().y();
    }
}

void LabelSlider::mouseMoveEvent(QMouseEvent*) {
    if (drag_start) {
        set_value(internal_value + (cursor().pos().x()-drag_start_x) + (drag_start_y-cursor().pos().y()));
        cursor().setPos(drag_start_x, drag_start_y);
        drag_proc = true;
    }
}

void LabelSlider::mouseReleaseEvent(QMouseEvent*) {
    if (drag_start) {
        qApp->restoreOverrideCursor();
        drag_start = false;
        if (drag_proc) {
            // send undo event
            ValueChangeCommand* vcc = new ValueChangeCommand();
            vcc->source = this;
            vcc->old_val = drag_start_value;
            vcc->new_val = internal_value;
            undo_stack.push(vcc);

            drag_proc = false;
        } else {
            double d = QInputDialog::getDouble(this, "Set Value", "New value:", internal_value);
            if (d != internal_value) {
                ValueChangeCommand* vcc = new ValueChangeCommand();
                vcc->source = this;
                vcc->old_val = internal_value;

                set_value(d);

                vcc->new_val = internal_value;
                undo_stack.push(vcc);
            }
        }
    }
}

ValueChangeCommand::ValueChangeCommand() : done(true) {}

void ValueChangeCommand::undo() {
    source->set_value(old_val);
    done = false;
}

void ValueChangeCommand::redo() {
    if (!done) {
        source->set_value(new_val);
    }
}

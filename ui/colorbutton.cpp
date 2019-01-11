#include "colorbutton.h"

#include "project/undo.h"

#include <QColorDialog>

ColorButton::ColorButton(QWidget *parent)
    : QPushButton(parent), color(Qt::white) {
    set_button_color();
    connect(this, SIGNAL(clicked(bool)), this, SLOT(open_dialog()));
}

QColor ColorButton::get_color() {
    return color;
}

void ColorButton::set_color(QColor c) {
	previousColor = color;
    color = c;
	set_button_color();
}

const QColor &ColorButton::getPreviousValue() {
	return previousColor;
}

void ColorButton::set_button_color() {
    QPalette pal = palette();
    pal.setColor(QPalette::Button, color);
    setPalette(pal);
}

void ColorButton::open_dialog() {
    QColor new_color = QColorDialog::getColor(color, NULL, tr("Set Color"));
	if (new_color.isValid() && color != new_color) {
		set_color(new_color);
        set_button_color();
		emit color_changed();
    }
}

ColorCommand::ColorCommand(ColorButton* s, QColor o, QColor n)
    : sender(s), old_color(o), new_color(n) {}

void ColorCommand::undo() {
    sender->set_color(old_color);
}

void ColorCommand::redo() {
    sender->set_color(new_color);
}

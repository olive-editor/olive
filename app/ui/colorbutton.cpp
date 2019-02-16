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
	QColor new_color = QColorDialog::getColor(color, nullptr, tr("Set Color"));
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

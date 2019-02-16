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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>
#include <QColor>
#include <QUndoCommand>

class ColorButton : public QPushButton {
    Q_OBJECT
public:
    ColorButton(QWidget* parent = 0);
    QColor get_color();
    void set_color(QColor c);
	const QColor& getPreviousValue();
private:
    QColor color;
	QColor previousColor;
    void set_button_color();
signals:
    void color_changed();
private slots:
    void open_dialog();
};

class ColorCommand : public QUndoCommand {
public:
    ColorCommand(ColorButton* s, QColor o, QColor n);
    void undo();
    void redo();
private:
    ColorButton* sender;
    QColor old_color;
    QColor new_color;
};

#endif // COLORBUTTON_H

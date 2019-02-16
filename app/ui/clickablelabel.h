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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

/**
 * @brief The ClickableLabel class
 *
 * Simple QLabel-derived class that emits a clicked() signal when the widget receives a mouse press event.
 */
class ClickableLabel : public QLabel {
	Q_OBJECT
public:
	ClickableLabel(QWidget * parent = 0, Qt::WindowFlags f = 0);
	ClickableLabel(const QString & text, QWidget * parent = 0, Qt::WindowFlags f = 0);
	void mousePressEvent(QMouseEvent *ev);
signals:
	void clicked();
};

#endif // CLICKABLELABEL_H

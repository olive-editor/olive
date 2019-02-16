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

#ifndef VIEWERCONTAINER_H
#define VIEWERCONTAINER_H

#include <QWidget>
class Viewer;
class ViewerWidget;
class QScrollBar;

class ViewerContainer : public QWidget
{
	Q_OBJECT
public:
	explicit ViewerContainer(QWidget *parent = 0);
	~ViewerContainer();

	bool fit;
	double zoom;

	void dragScrollPress(const QPoint&);
	void dragScrollMove(const QPoint&);
	void parseWheelEvent(QWheelEvent* event);

	Viewer* viewer;
	ViewerWidget* child;
	void adjust();

	// manually moves scrollbars into the correct position
	void adjust_scrollbars();

protected:
	void resizeEvent(QResizeEvent *event);

signals:

public slots:

private slots:
	void scroll_changed();

private:
	int drag_start_x;
	int drag_start_y;
	int horiz_start;
	int vert_start;
	QScrollBar* horizontal_scrollbar;
	QScrollBar* vertical_scrollbar;
};

#endif // VIEWERCONTAINER_H

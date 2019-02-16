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

#include "resizablescrollbar.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

#include "debug.h"

#define RESIZE_HANDLE_SIZE 10

ResizableScrollBar::ResizableScrollBar(QWidget *parent) :
	QScrollBar(parent),
	resize_init(false),
	resize_proc(false)
{
	setSingleStep(20);
	setMaximum(0);
	setMouseTracking(true);
}

bool ResizableScrollBar::is_resizing() {
	return resize_proc;
}

void ResizableScrollBar::resizeEvent(QResizeEvent *event) {
	setPageStep(event->size().width());
}

void ResizableScrollBar::mousePressEvent(QMouseEvent *e) {
	if (resize_init) {
		QStyleOptionSlider opt;
		initStyleOption(&opt);

		QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
											   QStyle::SC_ScrollBarSlider, this);

		resize_proc = true;
		resize_start = e->pos().x();

		resize_start_max = maximum();
		resize_start_width = sr.width();
	} else {
		QScrollBar::mousePressEvent(e);
	}
}

void ResizableScrollBar::mouseMoveEvent(QMouseEvent *e) {
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
										   QStyle::SC_ScrollBarSlider, this);
	QRect gr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
										   QStyle::SC_ScrollBarGroove, this);

	if (resize_proc) {
		int diff = (e->pos().x() - resize_start);
		if (resize_top) diff = -diff;
		double scale = double(sr.width())/double(sr.width()+diff);
		if (!qIsInf(scale) && !qIsNull(scale)) {
			emit resize_move(scale);
			resize_start = e->pos().x();

			if (resize_top) {
				int slider_min = gr.x();
				int slider_max = gr.right() - (sr.width()+diff);
				int val = QStyle::sliderValueFromPosition(minimum(), maximum(), e->pos().x() - slider_min, slider_max - slider_min, opt.upsideDown);

				setValue(val);
			} else {
				setValue(qRound(value() * scale));
			}
		}
	} else {
		bool new_resize_init = false;

		if ((orientation() == Qt::Horizontal && e->pos().x() > sr.left()-RESIZE_HANDLE_SIZE && e->pos().x() < sr.left()+RESIZE_HANDLE_SIZE)
				|| (orientation() == Qt::Vertical && e->pos().y() > sr.top()-RESIZE_HANDLE_SIZE && e->pos().y() < sr.top()+RESIZE_HANDLE_SIZE)) {
			new_resize_init = true;
			resize_top = true;
		} else if ((orientation() == Qt::Horizontal && e->pos().x() > sr.right()-RESIZE_HANDLE_SIZE && e->pos().x() < sr.right()+RESIZE_HANDLE_SIZE)
				   || (orientation() == Qt::Vertical && e->pos().y() > sr.bottom()-RESIZE_HANDLE_SIZE && e->pos().y() < sr.bottom()+RESIZE_HANDLE_SIZE)) {
			new_resize_init = true;
			resize_top = false;
		}

		if (resize_init != new_resize_init) {
			if (new_resize_init) {
				setCursor(Qt::SizeHorCursor);
			} else {
				unsetCursor();
			}
			resize_init = new_resize_init;
		}

		QScrollBar::mouseMoveEvent(e);
	}
}

void ResizableScrollBar::mouseReleaseEvent(QMouseEvent *e) {
	if (resize_proc) {
		resize_proc = false;
	} else {
		QScrollBar::mouseReleaseEvent(e);
	}
}

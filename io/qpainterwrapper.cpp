/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "qpainterwrapper.h"

#include <QPainter>
#include "debug.h"

// exposes several QPainter functions to QJSEngine

QPainterWrapper painter_wrapper;

QPainterWrapper::QPainterWrapper() {}

QColor get_color_from_string(const QString& s) {
	dout << s;

	// workaround for alpha
	if (s.at(0) == '#' && s.length() == 9) {
		QColor color(s.left(7));
		color.setAlpha(s.mid(7).toInt(NULL, 16));
		return color;
	} else {
		return QColor(s);
	}
}

void QPainterWrapper::fill(const QString& c) {
	img->fill(get_color_from_string(c));
}

void QPainterWrapper::fillRect(int x, int y, int width, int height, const QString& brush) {
	painter->fillRect(x, y, width, height, get_color_from_string(brush));
}

void QPainterWrapper::drawRect(int x, int y, int width, int height) {
	painter->drawRect(x, y, width, height);
}

void QPainterWrapper::setPen(const QString& pen) {
	painter->setPen(get_color_from_string(pen));
}

void QPainterWrapper::setBrush(const QString& brush) {
	painter->setBrush(get_color_from_string(brush));
}

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
		color.setAlpha(s.mid(7).toInt(nullptr, 16));
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

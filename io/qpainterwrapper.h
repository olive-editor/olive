#ifndef QPAINTERWRAPPER_H
#define QPAINTERWRAPPER_H

#include <QObject>

class QPainter;

class QPainterWrapper : public QObject {
	Q_OBJECT
public:
	QPainterWrapper();
	QImage* img;
	QPainter* painter;
public slots:
	void fill(const QString& color);
	void fillRect(int x, int y, int width, int height, const QString& brush);
	void drawRect(int x, int y, int width, int height);
	void setPen(const QString& pen);
	void setBrush(const QString& brush);
};

extern QPainterWrapper painter_wrapper;

#endif // QPAINTERWRAPPER_H

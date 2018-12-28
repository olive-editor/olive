#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>

class EffectRow;

QColor get_curve_color(int index, int length);

class GraphView : public QWidget {
	Q_OBJECT
public:
    GraphView(QWidget* parent = 0);

    void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

	void set_row(EffectRow* r);
signals:
	void x_scroll_changed(int);
	void y_scroll_changed(int);
	void zoom_changed(double);
private:
	int x_scroll;
	int y_scroll;
	bool mousedown;
	int start_x;
	int start_y;
	double zoom;

	void set_scroll_x(int s);
	void set_scroll_y(int s);

	int get_screen_x(double);
	int get_screen_y(double);

	EffectRow* row;
};

#endif // GRAPHVIEW_H

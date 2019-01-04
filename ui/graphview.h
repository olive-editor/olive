#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>
#include <QVector>

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

	void set_selected_keyframe_type(int type);
	void set_field_visibility(int field, bool b);
signals:
	void x_scroll_changed(int);
	void y_scroll_changed(int);
	void zoom_changed(double);
	void selection_changed(bool, int);
private:
	int x_scroll;
	int y_scroll;
	bool mousedown;
	int start_x;
	int start_y;
	double zoom;

	void set_scroll_x(int s);
	void set_scroll_y(int s);
	void set_zoom(double z);

	int get_screen_x(double);
	int get_screen_y(double);

	QVector<bool> field_visibility;

	QVector<int> selected_keys;
	QVector<int> selected_keys_fields;
	QVector<long> selected_keys_old_vals;
	QVector<double> selected_keys_old_doubles;

	double old_handle_x;
	double old_handle_y;

	bool moved_keys;

	int current_handle;

	void draw_lines(QPainter &p, bool vert);
	void draw_line_text(QPainter &p, bool vert, int line_no, int line_pos, int next_line_pos);

	EffectRow* row;
private slots:
	void show_context_menu(const QPoint& pos);
	void reset_view();
};

#endif // GRAPHVIEW_H

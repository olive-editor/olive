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

#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>
#include <QVector>

class EffectRow;
class EffectField;

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

	void delete_selected_keys();
	void select_all();
signals:
	void x_scroll_changed(int);
	void y_scroll_changed(int);
    void zoom_changed(double, double);
	void selection_changed(bool, int);
private:
	int x_scroll;
	int y_scroll;
	bool mousedown;
	int start_x;
	int start_y;

    double x_zoom;
    double y_zoom;

	void set_scroll_x(int s);
	void set_scroll_y(int s);
    void set_zoom(double xz, double yz);

	int get_screen_x(double);
	int get_screen_y(double);
	long get_value_x(int);
	double get_value_y(int);

	void selection_update();

	QVector<bool> field_visibility;

	QVector<int> selected_keys;
	QVector<int> selected_keys_fields;
	QVector<long> selected_keys_old_vals;
	QVector<double> selected_keys_old_doubles;

	double old_pre_handle_x;
	double old_pre_handle_y;
	double old_post_handle_x;
	double old_post_handle_y;

	int handle_field;
	int handle_index;

	bool moved_keys;

	int current_handle;

	void draw_lines(QPainter &p, bool vert);
	void draw_line_text(QPainter &p, bool vert, int line_no, int line_pos, int next_line_pos);

	EffectRow* row;

	bool rect_select;
	int rect_select_x;
	int rect_select_y;
	int rect_select_w;
	int rect_select_h;
	int rect_select_offset;

	long visible_in;

	bool click_add;
	bool click_add_proc;
	EffectField* click_add_field;
	int click_add_key;
	int click_add_type;
private slots:
	void show_context_menu(const QPoint& pos);
	void reset_view();
	void set_view_to_selection();
	void set_view_to_all();
	void set_view_to_rect(int x1, double y1, int x2, double y2);
};

#endif // GRAPHVIEW_H

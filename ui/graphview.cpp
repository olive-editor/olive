#include "graphview.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QMenu>
#include <cfloat>

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "project/sequence.h"
#include "project/effectrow.h"
#include "project/effectfield.h"
#include "ui/keyframedrawing.h"
#include "project/undo.h"
#include "project/effect.h"
#include "project/clip.h"
#include "ui/rectangleselect.h"

#include "debug.h"

#define GRAPH_ZOOM_SPEED 0.05
#define GRAPH_SIZE 100
#define BEZIER_HANDLE_SIZE 3
#define BEZIER_LINE_SIZE 2

#define BEZIER_HANDLE_NONE 1
#define BEZIER_HANDLE_PRE 2
#define BEZIER_HANDLE_POST 3

QColor get_curve_color(int index, int length) {
	QColor c;
	int hue = qRound((double(index)/double(length))*255);
	c.setHsv(hue, 255, 255);
	return c;
}

GraphView::GraphView(QWidget* parent) :
	QWidget(parent),
	x_scroll(0),
	y_scroll(0),
	mousedown(false),
	zoom(1.0),
	row(nullptr),
	moved_keys(false),
	current_handle(BEZIER_HANDLE_NONE),
	rect_select(false),
	visible_in(0),
	click_add_proc(false)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void GraphView::show_context_menu(const QPoint& pos) {
	QMenu menu(this);

	QAction* zoom_to_selection = menu.addAction(tr("Zoom to Selection"));
	if (selected_keys.size() == 0 || row == nullptr) {
		zoom_to_selection->setEnabled(false);
	} else {
		connect(zoom_to_selection, SIGNAL(triggered(bool)), this, SLOT(set_view_to_selection()));
	}

	QAction* zoom_to_all = menu.addAction(tr("Zoom to Show All"));
	if (row == nullptr) {
		zoom_to_all->setEnabled(false);
	} else {
		connect(zoom_to_all, SIGNAL(triggered(bool)), this, SLOT(set_view_to_all()));
	}

	menu.addSeparator();

	QAction* reset_action = menu.addAction(tr("Reset View"));
	if (row == nullptr) {
		reset_action->setEnabled(false);
	} else {
		connect(reset_action, SIGNAL(triggered(bool)), this, SLOT(reset_view()));
	}

	menu.exec(mapToGlobal(pos));
}

void GraphView::reset_view() {
	zoom = 1.0;
	set_scroll_x(0);
	set_scroll_y(0);
	emit zoom_changed(zoom);
	update();
}

void GraphView::set_view_to_selection() {
	if (row != nullptr && selected_keys.size() > 0) {
		long min_time = LONG_MAX;
		long max_time = LONG_MIN;
		double min_dbl = DBL_MAX;
		double max_dbl = DBL_MIN;
		for (int i=0;i<selected_keys.size();i++) {
			const EffectKeyframe& key = row->field(selected_keys_fields.at(i))->keyframes.at(selected_keys.at(i));
			min_time = qMin(key.time, min_time);
			max_time = qMax(key.time, max_time);
			min_dbl = qMin(key.data.toDouble(), min_dbl);
			max_dbl = qMax(key.data.toDouble(), max_dbl);
		}

        min_time -= row->parent_effect->parent_clip->clip_in;
        max_time -= row->parent_effect->parent_clip->clip_in;

		set_view_to_rect(min_time, min_dbl, max_time, max_dbl);
	}
}

void GraphView::set_view_to_all() {
	if (row != nullptr) {
		bool can_set = false;

		long min_time = LONG_MAX;
		long max_time = LONG_MIN;
		double min_dbl = DBL_MAX;
		double max_dbl = DBL_MIN;
		for (int i=0;i<row->fieldCount();i++) {
			for (int j=0;j<row->field(i)->keyframes.size();j++) {
				const EffectKeyframe& key = row->field(i)->keyframes.at(j);
				min_time = qMin(key.time, min_time);
				max_time = qMax(key.time, max_time);
				min_dbl = qMin(key.data.toDouble(), min_dbl);
				max_dbl = qMax(key.data.toDouble(), max_dbl);
				can_set = true;
			}
		}
		if (can_set) {
            min_time -= row->parent_effect->parent_clip->clip_in;
            max_time -= row->parent_effect->parent_clip->clip_in;

			set_view_to_rect(min_time, min_dbl, max_time, max_dbl);
		}
	}
}

void GraphView::set_view_to_rect(int x1, double y1, int x2, double y2) {
	double padding = 1.5;
	double x_diff = double(x2 - x1);
	double y_diff = (y2 - y1);
	double x_diff_padded = (x_diff+10)*padding;
	double y_diff_padded = (y_diff+10)*padding;
	set_zoom(qMin(double(width()) / x_diff_padded, double(height()) / y_diff_padded));

	set_scroll_x(qRound((double(x1) - ((x_diff_padded-x_diff)/2))*zoom));
	set_scroll_y(qRound((double(y1) - ((y_diff_padded-y_diff)/2))*zoom));
}

void GraphView::draw_line_text(QPainter &p, bool vert, int line_no, int line_pos, int next_line_pos) {
	// draws last line's text
	QString str = QString::number(line_no*GRAPH_SIZE);
	int text_sz = vert ? fontMetrics().height() : fontMetrics().width(str);
	if (text_sz < (next_line_pos - line_pos)) {
		QRect text_rect = vert ? QRect(0, line_pos-50, 50, 50) : QRect(line_pos, height()-50, 50, 50);
		p.drawText(text_rect, Qt::AlignBottom | Qt::AlignLeft, str);
	}
}

void GraphView::draw_lines(QPainter& p, bool vert) {
	int last_line = INT_MIN;
	int last_line_x = INT_MIN;
	int lim = vert ? height() : width();
	int scroll = vert ? y_scroll : x_scroll;
	for (int i=0;i<lim;i++) {
		int scroll_offs = vert ? height() - i + scroll : i + scroll;
		int this_line = qFloor(scroll_offs/(GRAPH_SIZE*zoom));
		if (vert) this_line++;
		if (this_line != last_line) {
			if (vert) {
				p.drawLine(0, i, width(), i);
			} else {
				p.drawLine(i, 0, i, height());
			}

			draw_line_text(p, vert, last_line, last_line_x, i);

			last_line_x = i;
			last_line = this_line;
		}
	}
	draw_line_text(p, vert, last_line, last_line_x, width());
}

QVector<int> sort_keys_from_field(EffectField* field) {
	QVector<int> sorted_keys;
	for (int k=0;k<field->keyframes.size();k++) {
		bool inserted = false;
		for (int j=0;j<sorted_keys.size();j++) {
			if (field->keyframes.at(sorted_keys.at(j)).time > field->keyframes.at(k).time) {
				sorted_keys.insert(j, k);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			sorted_keys.append(k);
		}
	}
	return sorted_keys;
}

void GraphView::paintEvent(QPaintEvent *) {
	QPainter p(this);

	if (panel_sequence_viewer->seq != nullptr) {
		// draw grid lines

		p.setPen(Qt::gray);

		draw_lines(p, true);
		draw_lines(p, false);

		// draw keyframes
		if (row != nullptr) {
			QPen line_pen;
			line_pen.setWidth(BEZIER_LINE_SIZE);

			for (int i=row->fieldCount()-1;i>=0;i--) {
				EffectField* field = row->field(i);

				if (field->type == EFFECT_FIELD_DOUBLE && field_visibility.at(i)) {
					// sort keyframes by time
					QVector<int> sorted_keys = sort_keys_from_field(field);

					int last_key_x = 0;
					int last_key_y = 0;

					// draw lines
					for (int j=0;j<sorted_keys.size();j++) {
						const EffectKeyframe& key = field->keyframes.at(sorted_keys.at(j));

                        int key_x = get_screen_x(key.time);
						int key_y = get_screen_y(key.data.toDouble());

						line_pen.setColor(get_curve_color(i, row->fieldCount()));
						p.setPen(line_pen);
						if (j == 0) {
							p.drawLine(0, key_y, key_x, key_y);
						} else {
							const EffectKeyframe& last_key = field->keyframes.at(sorted_keys.at(j-1));

							double pre_handle = field->get_validated_keyframe_handle(sorted_keys.at(j), false);
							double last_post_handle = field->get_validated_keyframe_handle(sorted_keys.at(j-1), true);

							if (last_key.type == EFFECT_KEYFRAME_HOLD) {
								// hold
								p.drawLine(last_key_x, last_key_y, key_x, last_key_y);
								p.drawLine(key_x, last_key_y, key_x, key_y);
							} else if (last_key.type == EFFECT_KEYFRAME_BEZIER || key.type == EFFECT_KEYFRAME_BEZIER) {
								QPainterPath bezier_path;
								bezier_path.moveTo(last_key_x, last_key_y);
								if (last_key.type == EFFECT_KEYFRAME_BEZIER && key.type == EFFECT_KEYFRAME_BEZIER) {
									// cubic bezier
									bezier_path.cubicTo(
												QPointF(last_key_x+last_post_handle*zoom, last_key_y-last_key.post_handle_y*zoom),
												QPointF(key_x+pre_handle*zoom, key_y-key.pre_handle_y*zoom),
												QPointF(key_x, key_y)
											);
								} else if (key.type == EFFECT_KEYFRAME_LINEAR) { // quadratic bezier
									// last keyframe is the bezier one
									bezier_path.quadTo(
												QPointF(last_key_x+last_post_handle*zoom, last_key_y-last_key.post_handle_y*zoom),
												QPointF(key_x, key_y)
											);
								} else {
									// this keyframe is the bezier one
									bezier_path.quadTo(
												QPointF(key_x+pre_handle*zoom, key_y-key.pre_handle_y*zoom),
												QPointF(key_x, key_y)
											);
								}
								p.drawPath(bezier_path);
							} else {
								// linear
								p.drawLine(last_key_x, last_key_y, key_x, key_y);
							}
						}
						last_key_x = key_x;
						last_key_y = key_y;
					}
					if (last_key_x < width()) p.drawLine(last_key_x, last_key_y, width(), last_key_y);

					// draw keys
					for (int j=0;j<sorted_keys.size();j++) {
						const EffectKeyframe& key = field->keyframes.at(sorted_keys.at(j));

                        int key_x = get_screen_x(key.time);
						int key_y = get_screen_y(key.data.toDouble());

						if (key.type == EFFECT_KEYFRAME_BEZIER) {
							p.setPen(Qt::gray);

							// pre handle line
							QPointF pre_point(key_x + key.pre_handle_x*zoom, key_y - key.pre_handle_y*zoom);
							p.drawLine(pre_point, QPointF(key_x, key_y));
							p.drawEllipse(pre_point, BEZIER_HANDLE_SIZE, BEZIER_HANDLE_SIZE);

							// post handle line
							QPointF post_point(key_x + key.post_handle_x*zoom, key_y - key.post_handle_y*zoom);
							p.drawLine(post_point, QPointF(key_x, key_y));
							p.drawEllipse(post_point, BEZIER_HANDLE_SIZE, BEZIER_HANDLE_SIZE);
						}

						bool selected = false;
						for (int k=0;k<selected_keys.size();k++) {
							if (selected_keys.at(k) == sorted_keys.at(j) && selected_keys_fields.at(k) == i) {
								selected = true;
								break;
							}
						}

						draw_keyframe(p, key.type, key_x, key_y, selected);
					}
				}
			}
		}

		// draw playhead
		p.setPen(Qt::red);
        int playhead_x = qRound((double(panel_sequence_viewer->seq->playhead - visible_in)*zoom) - x_scroll);
		p.drawLine(playhead_x, 0, playhead_x, height());

		if (rect_select) {
			draw_selection_rectangle(p, QRect(rect_select_x, rect_select_y, rect_select_w, rect_select_h));
			p.setBrush(Qt::NoBrush);
		}
	}

	p.setPen(Qt::white);

	QRect border = rect();
	border.setWidth(border.width()-1);
	border.setHeight(border.height()-1);
	p.drawRect(border);
}

void GraphView::mousePressEvent(QMouseEvent *event) {
	if (row != nullptr) {
		mousedown = true;
		start_x = event->pos().x();
		start_y = event->pos().y();

		// selecting
		int sel_key = -1;
		int sel_key_field = -1;
		current_handle = BEZIER_HANDLE_NONE;

		if (click_add && (event->buttons() & Qt::LeftButton)) {
			selected_keys.clear();
			selected_keys_fields.clear();

			EffectKeyframe key;
			key.time = get_value_x(event->pos().x());
			key.data = get_value_y(event->pos().y());
			key.type = click_add_type;
			click_add_key = click_add_field->keyframes.size();
			click_add_field->keyframes.append(key);
			update_ui(false);
			click_add_proc = true;
		} else {
			for (int i=0;i<row->fieldCount();i++) {
				EffectField* field = row->field(i);
				if (field->type == EFFECT_FIELD_DOUBLE && field_visibility.at(i)) {
					for (int j=0;j<field->keyframes.size();j++) {
						const EffectKeyframe& key = field->keyframes.at(j);
                        int key_x = get_screen_x(key.time);
						int key_y = get_screen_y(key.data.toDouble());
						if (event->pos().x() > key_x-KEYFRAME_SIZE
								&& event->pos().x() < key_x+KEYFRAME_SIZE
								&& event->pos().y() > key_y-KEYFRAME_SIZE
								&& event->pos().y() < key_y+KEYFRAME_SIZE) {
							sel_key = j;
							sel_key_field = i;
							break;
						} else {
							// selecting a handle
							QPointF pre_point(key_x + key.pre_handle_x*zoom, key_y - key.pre_handle_y*zoom);
							QPointF post_point(key_x + key.post_handle_x*zoom, key_y - key.post_handle_y*zoom);
							if (event->pos().x() > pre_point.x()-BEZIER_HANDLE_SIZE
									&& event->pos().x() < pre_point.x()+BEZIER_HANDLE_SIZE
									&& event->pos().y() > pre_point.y()-BEZIER_HANDLE_SIZE
									&& event->pos().y() < pre_point.y()+BEZIER_HANDLE_SIZE) {
								current_handle = BEZIER_HANDLE_PRE;
							} else if (event->pos().x() > post_point.x()-BEZIER_HANDLE_SIZE
									   && event->pos().x() < post_point.x()+BEZIER_HANDLE_SIZE
									   && event->pos().y() > post_point.y()-BEZIER_HANDLE_SIZE
									   && event->pos().y() < post_point.y()+BEZIER_HANDLE_SIZE) {
								current_handle = BEZIER_HANDLE_POST;
							}

							if (current_handle != BEZIER_HANDLE_NONE) {
								sel_key = j;
								sel_key_field = i;
								handle_index = j;
								handle_field = i;
								old_pre_handle_x = key.pre_handle_x;
								old_pre_handle_y = key.pre_handle_y;
								old_post_handle_x = key.post_handle_x;
								old_post_handle_y = key.post_handle_y;
								break;
							}
						}
					}
				}
				if (sel_key > -1) break;
			}

			bool already_selected = false;
			if (sel_key > -1) {
				for (int i=0;i<selected_keys.size();i++) {
					if (selected_keys.at(i) == sel_key && selected_keys_fields.at(i) == sel_key_field) {
						if ((event->modifiers() & Qt::ShiftModifier) && current_handle == BEZIER_HANDLE_NONE) {
							selected_keys.removeAt(i);
							selected_keys_fields.removeAt(i);
						}
						already_selected = true;
						break;
					}
				}
			}
			if (!already_selected) {
				if (!(event->modifiers() & Qt::ShiftModifier)) {
					selected_keys.clear();
					selected_keys_fields.clear();
				}
				if (sel_key > -1) {
					selected_keys.append(sel_key);
					selected_keys_fields.append(sel_key_field);
				} else {
					rect_select = true;
					rect_select_x = event->pos().x();
					rect_select_y = event->pos().y();
					rect_select_w = 0;
					rect_select_h = 0;
					rect_select_offset = selected_keys.size();
				}
			}

			selection_update();
		}
	}
}

void GraphView::mouseMoveEvent(QMouseEvent *event) {
	if (!mousedown || !click_add) unsetCursor();
	if (mousedown) {
		if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
			set_scroll_x(x_scroll + start_x - event->pos().x());
			set_scroll_y(y_scroll + event->pos().y() - start_y);
			start_x = event->pos().x();
			start_y = event->pos().y();
			update();
		} else if (click_add_proc) {
			click_add_field->keyframes[click_add_key].time = get_value_x(event->pos().x());
			click_add_field->keyframes[click_add_key].data = get_value_y(event->pos().y());
			update_ui(false);
		} else if (rect_select) {
			rect_select_w = event->pos().x() - rect_select_x;
			rect_select_h = event->pos().y() - rect_select_y;

			selected_keys.resize(rect_select_offset);
			selected_keys_fields.resize(rect_select_offset);

			for (int i=0;i<row->fieldCount();i++) {
				EffectField* f = row->field(i);
				for (int j=0;j<f->keyframes.size();j++) {
					bool already_selected = false;
					for (int k=0;k<selected_keys.size();k++) {
						if (selected_keys.at(k) == j && selected_keys_fields.at(k) == i) {
							already_selected = true;
							break;
						}
					}

					if (!already_selected) {
						QPoint key_screen_point(get_screen_x(f->keyframes.at(j).time), get_screen_y(f->keyframes.at(j).data.toDouble()));
						QRect select_rect(rect_select_x, rect_select_y, rect_select_w, rect_select_h);
						if (select_rect.contains(key_screen_point)) {
							selected_keys.append(j);
							selected_keys_fields.append(i);
						}
					}
				}
			}
			update();
		} else {
			switch (current_handle) {
			case BEZIER_HANDLE_NONE:
				for (int i=0;i<selected_keys.size();i++) {
					row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].time = qRound(selected_keys_old_vals.at(i) + (double(event->pos().x() - start_x)/zoom));
					if (event->modifiers() & Qt::ShiftModifier) {
						row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].data = selected_keys_old_doubles.at(i);
					} else {
						row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].data = qRound(selected_keys_old_doubles.at(i) + (double(start_y - event->pos().y())/zoom));
					}
				}
				moved_keys = true;
				update_ui(false);
				break;
			case BEZIER_HANDLE_PRE:
			case BEZIER_HANDLE_POST:
			{
				double new_pre_handle_x = old_pre_handle_x;
				double new_pre_handle_y = old_pre_handle_y;
				double new_post_handle_x = old_post_handle_x;
				double new_post_handle_y = old_post_handle_y;

				double x_diff = double(event->pos().x() - start_x)/zoom;
				double y_diff = double(start_y - event->pos().y())/zoom;

				if (current_handle == BEZIER_HANDLE_PRE) {
					new_pre_handle_x += x_diff;
					if (!(event->modifiers() & Qt::ShiftModifier)) new_pre_handle_y += y_diff;
					if (!(event->modifiers() & Qt::ControlModifier)) {
						new_post_handle_x = -new_pre_handle_x;
						new_post_handle_y = -new_pre_handle_y;
					}
				} else {
					new_post_handle_x += x_diff;
					if (!(event->modifiers() & Qt::ShiftModifier)) new_post_handle_y += y_diff;
					if (!(event->modifiers() & Qt::ControlModifier)) {
						new_pre_handle_x = -new_post_handle_x;
						new_pre_handle_y = -new_post_handle_y;
					}
				}

				EffectKeyframe& key = row->field(handle_field)->keyframes[handle_index];
				key.pre_handle_x = qMin(0.0, new_pre_handle_x);
				key.pre_handle_y = new_pre_handle_y;
				key.post_handle_x = qMax(0.0, new_post_handle_x);
				key.post_handle_y = new_post_handle_y;

				moved_keys = true;
				update_ui(false);
			}
				break;
			}
		}
	} else if (row != nullptr) {
		// clicking on the curve
		click_add = false;

		bool hovering_key = false;

		for (int i=0;i<row->fieldCount();i++) {
			for (int j=0;j<row->field(i)->keyframes.size();j++) {
				const EffectKeyframe& key = row->field(i)->keyframes.at(j);
                int key_x = get_screen_x(key.time);
				int key_y = get_screen_y(key.data.toDouble());
				QRect test_rect(
							key_x - KEYFRAME_SIZE,
							key_y - KEYFRAME_SIZE,
							KEYFRAME_SIZE+KEYFRAME_SIZE,
							KEYFRAME_SIZE+KEYFRAME_SIZE
						);
				QRect pre_rect(
							qRound(key_x + key.pre_handle_x*zoom - BEZIER_HANDLE_SIZE),
							qRound(key_y + key.pre_handle_y*zoom - BEZIER_HANDLE_SIZE),
							BEZIER_HANDLE_SIZE+BEZIER_HANDLE_SIZE,
							BEZIER_HANDLE_SIZE+BEZIER_HANDLE_SIZE
						);
				QRect post_rect(
							qRound(key_x + key.post_handle_x*zoom - BEZIER_HANDLE_SIZE),
							qRound(key_y + key.post_handle_y*zoom - BEZIER_HANDLE_SIZE),
							BEZIER_HANDLE_SIZE+BEZIER_HANDLE_SIZE,
							BEZIER_HANDLE_SIZE+BEZIER_HANDLE_SIZE
						);

				if (test_rect.contains(event->pos())
						|| pre_rect.contains(event->pos())
						|| post_rect.contains(event->pos())) {
					hovering_key = true;
					break;
				}
			}
		}

		if (!hovering_key) {
			for (int i=0;i<row->fieldCount();i++) {
				EffectField* f = row->field(i);
				if (field_visibility.at(i)) {
					QVector<int> sorted_keys = sort_keys_from_field(f);

					if (event->pos().x() <= get_screen_x(f->keyframes.at(sorted_keys.first()).time)) {
						int y_comp = get_screen_y(f->keyframes.at(sorted_keys.first()).data.toDouble());
						if (event->pos().y() >= y_comp-BEZIER_LINE_SIZE
								&& event->pos().y() <= y_comp+BEZIER_LINE_SIZE) {
	//						dout << "make an EARLY key on field" << i;
							click_add = true;
							click_add_type = f->keyframes.at(sorted_keys.first()).type;
						}
					} else if (event->pos().x() >= get_screen_x(f->keyframes.at(sorted_keys.last()).time)) {
						int y_comp = get_screen_y(f->keyframes.at(sorted_keys.last()).data.toDouble());
						if (event->pos().y() >= y_comp-BEZIER_LINE_SIZE
								&& event->pos().y() <= y_comp+BEZIER_LINE_SIZE) {
	//						dout << "make an LATE key on field" << i;
							click_add = true;
							click_add_type = f->keyframes.at(sorted_keys.last()).type;
						}
					} else {
						for (int j=1;j<sorted_keys.size();j++) {
							const EffectKeyframe& last_key = f->keyframes.at(sorted_keys.at(j-1));
							const EffectKeyframe& key = f->keyframes.at(sorted_keys.at(j));

							int last_key_x = get_screen_x(last_key.time);
							int key_x = get_screen_x(key.time);
							int last_key_y = get_screen_y(last_key.data.toDouble());
							int key_y = get_screen_y(key.data.toDouble());

							click_add_type = last_key.type;

							if (event->pos().x() >= last_key_x
									&& event->pos().x() <= key_x) {
								QRect mouse_rect(event->pos().x()-BEZIER_LINE_SIZE, event->pos().y()-BEZIER_LINE_SIZE, BEZIER_LINE_SIZE+BEZIER_LINE_SIZE, BEZIER_LINE_SIZE+BEZIER_LINE_SIZE);
								// NOTE: FILTHY copy/paste from paintEvent
								if (last_key.type == EFFECT_KEYFRAME_HOLD) {
									// hold
									if (event->pos().y() >= last_key_y-BEZIER_LINE_SIZE
											&& event->pos().y() <= last_key_y+BEZIER_LINE_SIZE) {
	//									dout << "make an HOLD key on field" << i << "after key" << j;
										click_add = true;
									}
								} else if (last_key.type == EFFECT_KEYFRAME_BEZIER || key.type == EFFECT_KEYFRAME_BEZIER) {
									QPainterPath bezier_path;
									bezier_path.moveTo(last_key_x, last_key_y);
									if (last_key.type == EFFECT_KEYFRAME_BEZIER && key.type == EFFECT_KEYFRAME_BEZIER) {
										// cubic bezier
										bezier_path.cubicTo(
													QPointF(last_key_x+last_key.post_handle_x*zoom, last_key_y-last_key.post_handle_y*zoom),
													QPointF(key_x+key.pre_handle_x*zoom, key_y-key.pre_handle_y*zoom),
													QPointF(key_x, key_y)
												);
									} else if (key.type == EFFECT_KEYFRAME_LINEAR) { // quadratic bezier
										// last keyframe is the bezier one
										bezier_path.quadTo(
													QPointF(last_key_x+last_key.post_handle_x*zoom, last_key_y-last_key.post_handle_y*zoom),
													QPointF(key_x, key_y)
												);
									} else {
										// this keyframe is the bezier one
										bezier_path.quadTo(
													QPointF(key_x+key.pre_handle_x*zoom, key_y-key.pre_handle_y*zoom),
													QPointF(key_x, key_y)
												);
									}
									if (bezier_path.intersects(mouse_rect)) {
	//									dout << "make an BEZIER key on field" << i << "after key" << j;
										click_add = true;
									}
								} else {
									// linear
									QPainterPath linear_path;
									linear_path.moveTo(last_key_x, last_key_y);
									linear_path.lineTo(key_x, key_y);
									if (linear_path.intersects(mouse_rect)) {
	//									dout << "make an LINEAR key on field" << i << "after key" << j;
										click_add = true;
									}
								}
							}
						}
					}
				}
				if (click_add) {
					click_add_field = f;
					setCursor(Qt::CrossCursor);
					break;
				}
			}
		}
	}
}

void GraphView::mouseReleaseEvent(QMouseEvent *) {
	if (click_add_proc) {
		undo_stack.push(new KeyframeFieldSet(click_add_field, click_add_key));
	} else if (moved_keys && selected_keys.size() > 0) {
		ComboAction* ca = new ComboAction();
		switch (current_handle) {
		case BEZIER_HANDLE_NONE:
			for (int i=0;i<selected_keys.size();i++) {
				EffectKeyframe& key = row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)];
				ca->append(new SetLong(&key.time, selected_keys_old_vals.at(i), key.time));
				ca->append(new SetQVariant(&key.data, selected_keys_old_doubles.at(i), key.data));
			}
			break;
		case BEZIER_HANDLE_PRE:
		case BEZIER_HANDLE_POST:
		{
			EffectKeyframe& key = row->field(handle_field)->keyframes[handle_index];
			ca->append(new SetDouble(&key.pre_handle_x, old_pre_handle_x, key.pre_handle_x));
			ca->append(new SetDouble(&key.pre_handle_y, old_pre_handle_y, key.pre_handle_y));
			ca->append(new SetDouble(&key.post_handle_x, old_post_handle_x, key.post_handle_x));
			ca->append(new SetDouble(&key.post_handle_y, old_post_handle_y, key.post_handle_y));
		}
			break;
		}
		undo_stack.push(ca);
	}
	moved_keys = false;
	mousedown = false;
	click_add = false;
	click_add_proc = false;
	if (rect_select) {
		rect_select = false;
		selection_update();
		update();
	}
}

void GraphView::wheelEvent(QWheelEvent *event) {
	bool redraw = false;
	bool shift = (event->modifiers() & Qt::ShiftModifier); // scroll instead of zoom

	if (shift) {
		// scroll
		set_scroll_x(x_scroll + event->angleDelta().x()/10);
		set_scroll_y(y_scroll + event->angleDelta().y()/10);
		redraw = true;
	} else {
		// set zoom
		if (event->angleDelta().y() != 0) {
			double zoom_diff = (GRAPH_ZOOM_SPEED*zoom);
			double new_zoom = (event->angleDelta().y() < 0) ? zoom - zoom_diff : zoom + zoom_diff;

			// center zoom on screen
			set_scroll_x(qRound((double(x_scroll)/zoom*new_zoom) + double(event->pos().x())*new_zoom - double(event->pos().x())*zoom));
			set_scroll_y(qRound((double(y_scroll)/zoom*new_zoom) + double(height()-event->pos().y())*new_zoom - double(height()-event->pos().y())*zoom));

			set_zoom(new_zoom);

			redraw = true;
		}
	}

	if (redraw) {
		update();
	}
}

void GraphView::set_row(EffectRow *r) {
	if (row != r) {
		selected_keys.clear();
		selected_keys_fields.clear();
		selected_keys_old_vals.clear();
		selected_keys_old_doubles.clear();
		emit selection_changed(false, -1);
		row = r;
		if (row != nullptr) {
			field_visibility.resize(row->fieldCount());
			for (int i=0;i<row->fieldCount();i++) {
				field_visibility[i] = row->field(i)->is_enabled();
			}
			visible_in = row->parent_effect->parent_clip->timeline_in;
			set_view_to_all();
		} else {
			update();
		}
	}
}

void GraphView::set_selected_keyframe_type(int type) {
	if (selected_keys.size() > 0) {
		ComboAction* ca = new ComboAction();
		for (int i=0;i<selected_keys.size();i++) {
			EffectKeyframe& key = row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)];
			ca->append(new SetInt(&key.type, type));
		}
		undo_stack.push(ca);
		update_ui(false);
	}
}

void GraphView::set_field_visibility(int field, bool b) {
	field_visibility[field] = b;
	update();
}

void GraphView::delete_selected_keys() {
	if (row != nullptr) {
		QVector<EffectField*> fields;
		for (int i=0;i<selected_keys_fields.size();i++) {
			fields.append(row->field(selected_keys_fields.at(i)));
		}
		delete_keyframes(fields, selected_keys);
	}
}

void GraphView::select_all() {
	if (row != nullptr) {
		selected_keys.clear();
		selected_keys_fields.clear();
		for (int i=0;i<row->fieldCount();i++) {
			EffectField* field = row->field(i);
			for (int j=0;j<field->keyframes.size();j++) {
				selected_keys.append(j);
				selected_keys_fields.append(i);
			}
		}
		selection_update();
	}
}

void GraphView::set_scroll_x(int s) {
	x_scroll = s;
	emit x_scroll_changed(x_scroll);
}

void GraphView::set_scroll_y(int s) {
	y_scroll = s;
	emit y_scroll_changed(y_scroll);
}

void GraphView::set_zoom(double z) {
	zoom = z;
    emit zoom_changed(zoom);
}

int GraphView::get_screen_x(double d) {
    if (row != nullptr) {
        d -= row->parent_effect->parent_clip->clip_in;
    }
    return qRound((d*zoom) - x_scroll);
}

int GraphView::get_screen_y(double d) {
	return qRound(height() + y_scroll - d*zoom);
}

long GraphView::get_value_x(int i) {
    long frame = qRound((i + x_scroll)/zoom);
    if (row != nullptr) {
        frame += row->parent_effect->parent_clip->clip_in;
    }
    return frame;
}

double GraphView::get_value_y(int i) {
	return double(height() + y_scroll - i)/zoom;
}

void GraphView::selection_update() {
	selected_keys_old_vals.clear();
	selected_keys_old_doubles.clear();

	int selected_key_type = -1;

	for (int i=0;i<selected_keys.size();i++) {
		const EffectKeyframe& key = row->field(selected_keys_fields.at(i))->keyframes.at(selected_keys.at(i));
		selected_keys_old_vals.append(key.time);
		selected_keys_old_doubles.append(key.data.toDouble());

		if (selected_key_type == -1) {
			selected_key_type = key.type;
		} else if (selected_key_type != key.type) {
			selected_key_type = -2;
		}
	}

	update();

	emit selection_changed(selected_keys.size() > 0, selected_key_type);
}

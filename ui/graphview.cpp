#include "graphview.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QMenu>

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
	row(NULL),
	moved_keys(false),
	current_handle(BEZIER_HANDLE_NONE),
	rect_select(false),
	visible_in(0)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void GraphView::show_context_menu(const QPoint& pos) {
	QMenu menu(this);

	QAction* reset_action = menu.addAction("Reset View");
	connect(reset_action, SIGNAL(triggered(bool)), this, SLOT(reset_view()));

	menu.exec(mapToGlobal(pos));
}

void GraphView::reset_view() {
	zoom = 1.0;
	set_scroll_x(0);
	set_scroll_y(0);
	emit zoom_changed(zoom);
	update();
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

void GraphView::paintEvent(QPaintEvent *event) {
	QPainter p(this);

	if (panel_sequence_viewer->seq != NULL) {
		// draw grid lines

		p.setPen(Qt::gray);

		draw_lines(p, true);
		draw_lines(p, false);

		// draw keyframes
		if (row != NULL) {
			QPen line_pen;
			line_pen.setWidth(BEZIER_LINE_SIZE);

			for (int i=row->fieldCount()-1;i>=0;i--) {
				EffectField* field = row->field(i);

				if (field->type == EFFECT_FIELD_DOUBLE && field_visibility.at(i)) {
					// sort keyframes by time
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

					int last_key_x, last_key_y;

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
							if (last_key.type == KEYFRAME_TYPE_HOLD) {
								// hold
								p.drawLine(last_key_x, last_key_y, key_x, last_key_y);
								p.drawLine(key_x, last_key_y, key_x, key_y);
							} else if (last_key.type == KEYFRAME_TYPE_BEZIER || key.type == KEYFRAME_TYPE_BEZIER) {
								QPainterPath bezier_path;
								bezier_path.moveTo(last_key_x, last_key_y);
								if (last_key.type == KEYFRAME_TYPE_BEZIER && key.type == KEYFRAME_TYPE_BEZIER) {
									// cubic bezier
									bezier_path.cubicTo(
												QPointF(last_key_x+last_key.post_handle_x*zoom, last_key_y-last_key.post_handle_y*zoom),
												QPointF(key_x+key.pre_handle_x*zoom, key_y-key.pre_handle_y*zoom),
												QPointF(key_x, key_y)
											);
								} else if (key.type == KEYFRAME_TYPE_LINEAR) { // quadratic bezier
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
								p.drawPath(bezier_path);
							} else {
								// linear
								p.drawLine(last_key_x, last_key_y, key_x, key_y);
							}
						}
						last_key_x = key_x;
						last_key_y = key_y;
					}

					// draw keys
					for (int j=0;j<sorted_keys.size();j++) {
						const EffectKeyframe& key = field->keyframes.at(sorted_keys.at(j));

						int key_x = get_screen_x(key.time);
						int key_y = get_screen_y(key.data.toDouble());

						if (key.type == KEYFRAME_TYPE_BEZIER) {
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
		int playhead_x = get_screen_x(panel_sequence_viewer->seq->playhead - visible_in);
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
	if (row != NULL) {
		mousedown = true;
		start_x = event->pos().x();
		start_y = event->pos().y();

		// selecting
		int sel_key = -1;
		int sel_key_field = -1;
		current_handle = BEZIER_HANDLE_NONE;

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
						if (event->pos().x() > pre_point.x()-BEZIER_HANDLE_SIZE
								&& event->pos().x() < pre_point.x()+BEZIER_HANDLE_SIZE
								&& event->pos().y() > pre_point.y()-BEZIER_HANDLE_SIZE
								&& event->pos().y() < pre_point.y()+BEZIER_HANDLE_SIZE) {
							sel_key = j;
							sel_key_field = i;
							old_handle_x = key.pre_handle_x;
							old_handle_y = key.pre_handle_y;
							current_handle = BEZIER_HANDLE_PRE;
							break;
						} else {
							QPointF post_point(key_x + key.post_handle_x*zoom, key_y - key.post_handle_y*zoom);
							if (event->pos().x() > post_point.x()-BEZIER_HANDLE_SIZE
									&& event->pos().x() < post_point.x()+BEZIER_HANDLE_SIZE
									&& event->pos().y() > post_point.y()-BEZIER_HANDLE_SIZE
									&& event->pos().y() < post_point.y()+BEZIER_HANDLE_SIZE) {
								sel_key = j;
								sel_key_field = i;
								old_handle_x = key.post_handle_x;
								old_handle_y = key.post_handle_y;
								current_handle = BEZIER_HANDLE_POST;
								break;
							}
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
					if ((event->modifiers() & Qt::ShiftModifier)) {
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

void GraphView::mouseMoveEvent(QMouseEvent *event) {
	unsetCursor();
	if (mousedown) {
		if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
			set_scroll_x(x_scroll + start_x - event->pos().x());
			set_scroll_y(y_scroll + event->pos().y() - start_y);
			start_x = event->pos().x();
			start_y = event->pos().y();
			update();
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
			bool shift = (event->modifiers() & Qt::ShiftModifier);
			switch (current_handle) {
			case BEZIER_HANDLE_NONE:
				for (int i=0;i<selected_keys.size();i++) {
					row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].time = qRound(selected_keys_old_vals.at(i) + (double(event->pos().x() - start_x)/zoom));
					if (shift) {
						row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].data = selected_keys_old_doubles.at(i);
					} else {
						row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].data = qRound(selected_keys_old_doubles.at(i) + (double(start_y - event->pos().y())/zoom));
					}
				}
				moved_keys = true;
				update_ui(false);
				break;
			case BEZIER_HANDLE_PRE:
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].pre_handle_x = old_handle_x + double(event->pos().x() - start_x)/zoom;
				if (shift) {
					row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].pre_handle_y = old_handle_y;
				} else {
					row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].pre_handle_y = old_handle_y + double(start_y - event->pos().y())/zoom;
				}
				moved_keys = true;
				update_ui(false);
				break;
			case BEZIER_HANDLE_POST:
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].post_handle_x = old_handle_x + double(event->pos().x() - start_x)/zoom;
				if (shift) {
					row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].post_handle_y = old_handle_y;
				} else {
					row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].post_handle_y = old_handle_y + double(start_y - event->pos().y())/zoom;
				}
				moved_keys = true;
				update_ui(false);
				break;
			}

		}
	}
}

void GraphView::mouseReleaseEvent(QMouseEvent *event) {
	if (moved_keys && selected_keys.size() > 0) {
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
		{
			EffectKeyframe& key = row->field(selected_keys_fields.last())->keyframes[selected_keys.last()];
			ca->append(new SetDouble(&key.pre_handle_x, old_handle_x, key.pre_handle_x));
			ca->append(new SetDouble(&key.pre_handle_y, old_handle_y, key.pre_handle_y));
		}
			break;
		case BEZIER_HANDLE_POST:
		{
			EffectKeyframe& key = row->field(selected_keys_fields.last())->keyframes[selected_keys.last()];
			ca->append(new SetDouble(&key.post_handle_x, old_handle_x, key.post_handle_x));
			ca->append(new SetDouble(&key.post_handle_y, old_handle_y, key.post_handle_y));
		}
			break;
		}
		undo_stack.push(ca);
	}
	moved_keys = false;
	mousedown = false;
	if (rect_select) {
		rect_select = false;
		selection_update();
		update();
	}
}

void GraphView::wheelEvent(QWheelEvent *event) {
	bool redraw = false;
	bool shift = (event->modifiers() & Qt::ShiftModifier); // scroll instead of zoom
	bool alt = (event->modifiers() & Qt::AltModifier); // horiz scroll instead of vert scroll

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
			set_scroll_x(qRound(x_scroll + double(event->pos().x())*new_zoom - double(event->pos().x())*zoom));
			set_scroll_y(qRound(y_scroll + double(height()-event->pos().y())*new_zoom - double(height()-event->pos().y())*zoom));

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
		if (row != NULL) {
			field_visibility.resize(row->fieldCount());
			field_visibility.fill(true);
			visible_in = row->parent_effect->parent_clip->timeline_in;
		}
		update();
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
	return (d*zoom) - x_scroll;
}

int GraphView::get_screen_y(double d) {
	return height() + y_scroll - d*zoom;
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

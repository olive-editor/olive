#include "graphview.h"

#include <QPainter>
#include <QMouseEvent>

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "project/sequence.h"
#include "project/effectrow.h"
#include "project/effectfield.h"
#include "ui/keyframedrawing.h"
#include "project/undo.h"
#include "project/effect.h"

#include "debug.h"

#define GRAPH_ZOOM_SPEED 0.05
#define GRAPH_SIZE 100
#define BEZIER_HANDLE_SIZE 3

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
	current_handle(BEZIER_HANDLE_NONE)
{
	setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
}

void GraphView::paintEvent(QPaintEvent *event) {
	QPainter p(this);

	if (panel_sequence_viewer->seq != NULL) {
		// draw grid lines

		//int graph_size = GRAPH_SIZE*zoom;
		bool draw_text = true;//(fontMetrics().height() < graph_size && fontMetrics().width("0000") < graph_size);

		p.setPen(Qt::gray);
		int i = 0;
		while (true) {
			int line_x = (i*GRAPH_SIZE*zoom) - x_scroll;
			if (line_x >= width()) {
				break;
			}
			if (line_x >= 0) {
				if (line_x > 0) p.drawLine(line_x, 0, line_x, height());
				if (draw_text) p.drawText(QRect(line_x, height()-50, 50, 50), Qt::AlignBottom | Qt::AlignLeft, QString::number(i*GRAPH_SIZE));
			}
			i++;
		}
		i = 0;
		while (true) {
			int line_y = height() - (i*GRAPH_SIZE*zoom) + y_scroll;
			if (line_y <= 0) {
				break;
			}
			if (line_y <= height()) {
				if (line_y < height()) p.drawLine(0, line_y, width(), line_y);
				if (draw_text) p.drawText(QRect(0, line_y-50, 50, 50), Qt::AlignBottom | Qt::AlignLeft, QString::number(i*GRAPH_SIZE));
			}
			i++;
		}

		// draw keyframes
		if (row != NULL) {
			QPen line_pen;
			line_pen.setWidth(2);

			for (int i=0;i<row->fieldCount();i++) {
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
												QPointF(last_key_x+last_key.post_handle_x, last_key_y+last_key.post_handle_y),
												QPointF(key_x+key.pre_handle_x, key_y+key.pre_handle_y),
												QPointF(key_x, key_y)
											);
								} else if (key.type == KEYFRAME_TYPE_LINEAR) { // quadratic bezier
									// last keyframe is the bezier one
									bezier_path.quadTo(
												QPointF(last_key_x+last_key.post_handle_x, last_key_y+last_key.post_handle_y),
												QPointF(key_x, key_y)
											);
								} else {
									// this keyframe is the bezier one
									bezier_path.quadTo(
												QPointF(key_x+key.pre_handle_x, key_y+key.pre_handle_y),
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
							QPointF pre_point(key_x + key.pre_handle_x*zoom, key_y + key.pre_handle_y*zoom);
							p.drawLine(pre_point, QPointF(key_x, key_y));
							p.drawEllipse(pre_point, BEZIER_HANDLE_SIZE, BEZIER_HANDLE_SIZE);

							// post handle line
							QPointF post_point(key_x + key.post_handle_x*zoom, key_y + key.post_handle_y*zoom);
							p.drawLine(post_point, QPointF(key_x, key_y));
							p.drawEllipse(post_point, BEZIER_HANDLE_SIZE, BEZIER_HANDLE_SIZE);
						}

						draw_keyframe(p, key.type, key_x, key_y, (selected_keys.contains(sorted_keys.at(j)) && selected_keys_fields.contains(i)));
					}
				}
			}
		}

		// draw playhead
		p.setPen(Qt::red);
		int playhead_x = get_screen_x(panel_sequence_viewer->seq->playhead);
		p.drawLine(playhead_x, 0, playhead_x, height());
	}

	p.setPen(Qt::white);

	QRect border = rect();
	border.setWidth(border.width()-1);
	border.setHeight(border.height()-1);
	p.drawRect(border);
}

void GraphView::mousePressEvent(QMouseEvent *event) {
	mousedown = true;
	start_x = event->pos().x();
	start_y = event->pos().y();

	// selecting
	int sel_key = -1;
	int sel_key_field = -1;
	if (!(event->modifiers() & Qt::ShiftModifier)) {
		selected_keys.clear();
		selected_keys_fields.clear();
	}
	current_handle = BEZIER_HANDLE_NONE;
	if (row != NULL) {
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
						QPointF pre_point(key_x + key.pre_handle_x*zoom, key_y + key.pre_handle_y*zoom);
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
							QPointF post_point(key_x + key.post_handle_x*zoom, key_y + key.post_handle_y*zoom);
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
		}
	}
	if (sel_key > -1) {
		selected_keys.append(sel_key);
		selected_keys_fields.append(sel_key_field);
	}

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

void GraphView::mouseMoveEvent(QMouseEvent *event) {
	if (mousedown) {
		if (event->buttons() & Qt::MiddleButton || panel_timeline->tool == TIMELINE_TOOL_HAND) {
			set_scroll_x(x_scroll + start_x - event->pos().x());
			set_scroll_y(y_scroll + event->pos().y() - start_y);
			start_x = event->pos().x();
			start_y = event->pos().y();
			update();
		} else {
			switch (current_handle) {
			case BEZIER_HANDLE_NONE:
				for (int i=0;i<selected_keys.size();i++) {
					row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].time = qRound(selected_keys_old_vals.at(i) + (double(event->pos().x() - start_x)/zoom));
					row->field(selected_keys_fields.at(i))->keyframes[selected_keys.at(i)].data = qRound(selected_keys_old_doubles.at(i) + (double(start_y - event->pos().y())/zoom));
				}
				moved_keys = true;
				update_ui(false);
				break;
			case BEZIER_HANDLE_PRE:
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].pre_handle_x = old_handle_x + double(event->pos().x() - start_x)/zoom;
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].pre_handle_y = old_handle_y + double(event->pos().y() - start_y)/zoom;
				moved_keys = true;
				update_ui(false);
				break;
			case BEZIER_HANDLE_POST:
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].post_handle_x = old_handle_x + double(event->pos().x() - start_x)/zoom;
				row->field(selected_keys_fields.last())->keyframes[selected_keys.last()].post_handle_y = old_handle_y + double(event->pos().y() - start_y)/zoom;
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
			if (event->angleDelta().y() < 0) zoom_diff = -zoom_diff;
			zoom += zoom_diff;
			emit zoom_changed(zoom);
			redraw = true;
		}
	}

	if (redraw) {
		update();
	}
}

void GraphView::set_row(EffectRow *r) {
	selected_keys.clear();
	selected_keys_fields.clear();
	selected_keys_old_vals.clear();
	selected_keys_old_doubles.clear();
	emit selection_changed(false, -1);
	row = r;
	if (row != NULL) {
		field_visibility.resize(row->fieldCount());
		field_visibility.fill(true);
	}
	update();
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

int GraphView::get_screen_x(double d) {
	return (d*zoom) - x_scroll;
}

int GraphView::get_screen_y(double d) {
	return height() + y_scroll - d*zoom;
}

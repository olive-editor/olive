#include "keyframeview.h"

#include "project/effect.h"
#include "ui/collapsiblewidget.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "project/clip.h"
#include "panels/timeline.h"
#include "ui/timelineheader.h"
#include "project/undo.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "project/sequence.h"
#include "panels/grapheditor.h"
#include "ui/keyframedrawing.h"
#include "ui/clickablelabel.h"
#include "ui/resizablescrollbar.h"

#include <QMouseEvent>
#include <QtMath>
#include <QMenu>

long KeyframeView::adjust_row_keyframe(EffectRow* row, long time) {
	return time-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in);
}

KeyframeView::KeyframeView(QWidget *parent) :
	QWidget(parent),
	visible_in(0),
	visible_out(0),
	mousedown(false),
	dragging(false),
	keys_selected(false),
	select_rect(false),
	x_scroll(0),
	y_scroll(0),
	scroll_drag(false)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void KeyframeView::show_context_menu(const QPoint& pos) {
	if (selected_fields.size() > 0) {
		QMenu menu(this);

		QAction* linear = menu.addAction("Linear");
		linear->setData(KEYFRAME_TYPE_LINEAR);
		QAction* bezier = menu.addAction("Bezier");
		bezier->setData(KEYFRAME_TYPE_BEZIER);
		QAction* hold = menu.addAction("Hold");
		hold->setData(KEYFRAME_TYPE_HOLD);
		menu.addSeparator();
		menu.addAction("Graph Editor");

		connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_set_key_type(QAction*)));
		menu.exec(mapToGlobal(pos));
	}
}

void KeyframeView::menu_set_key_type(QAction* a) {
	if (a->data().isNull()) {
		// load graph editor
		panel_graph_editor->show();
	} else {
		ComboAction* ca = new ComboAction();
		for (int i=0;i<selected_fields.size();i++) {
			EffectField* f = selected_fields.at(i);
			ca->append(new SetInt(&f->keyframes[selected_keyframes.at(i)].type, a->data().toInt()));
		}
		undo_stack.push(ca);
		update_ui(false);
	}
}

void KeyframeView::paintEvent(QPaintEvent*) {
	QPainter p(this);

	rowY.clear();
	rows.clear();

	if (panel_effect_controls->selected_clips.size() > 0) {
		visible_in = LONG_MAX;
		visible_out = 0;

		for (int j=0;j<panel_effect_controls->selected_clips.size();j++) {
			Clip* c = sequence->clips.at(panel_effect_controls->selected_clips.at(j));
			visible_in = qMin(visible_in, c->timeline_in);
			visible_out = qMax(visible_out, c->timeline_out);
		}

		for (int j=0;j<panel_effect_controls->selected_clips.size();j++) {
			Clip* c = sequence->clips.at(panel_effect_controls->selected_clips.at(j));
			for (int i=0;i<c->effects.size();i++) {
				Effect* e = c->effects.at(i);
				if (e->container->is_expanded()) {
					for (int j=0;j<e->row_count();j++) {
						EffectRow* row = e->row(j);

						ClickableLabel* label = row->label;
						QWidget* contents = e->container->contents;

						QVector<long> key_times;
						int keyframe_y = label->y() + (label->height()>>1) + mapFrom(panel_effect_controls, contents->mapTo(panel_effect_controls, contents->pos())).y() - e->container->title_bar->height()/* - y_scroll*/;
						for (int l=0;l<row->fieldCount();l++) {
							EffectField* f = row->field(l);
							for (int k=0;k<f->keyframes.size();k++) {
								if (!key_times.contains(f->keyframes.at(k).time)) {
									bool keyframe_selected = keyframeIsSelected(f, k);
									long keyframe_frame = adjust_row_keyframe(row, f->keyframes.at(k).time);
									draw_keyframe(p, f->keyframes.at(k).type, getScreenPointFromFrame(panel_effect_controls->zoom, keyframe_frame) - x_scroll, keyframe_y, keyframe_selected);
									key_times.append(f->keyframes.at(k).time);
								}
							}
						}

						rows.append(row);
						rowY.append(keyframe_y);
					}
				}
			}
		}

		int max_width = getScreenPointFromFrame(panel_effect_controls->zoom, visible_out - visible_in);
		if (max_width < width()) {
			p.fillRect(QRect(max_width, 0, width(), height()), QColor(0, 0, 0, 64));
		}
		panel_effect_controls->horizontalScrollBar->setMaximum(qMax(max_width - width(), 0));
		header->set_visible_in(visible_in);

		int playhead_x = getScreenPointFromFrame(panel_effect_controls->zoom, sequence->playhead-visible_in) - x_scroll;
		if (dragging && panel_timeline->snapped) {
			p.setPen(Qt::white);
		} else {
			p.setPen(Qt::red);
		}
		p.drawLine(playhead_x, 0, playhead_x, height());
	}

	if (select_rect) {
		draw_selection_rectangle(p, QRect(rect_select_x, rect_select_y, rect_select_w, rect_select_h));
	}

	/*if (mouseover && mouseover_row < rowY.size()) {
		draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, mouseover_frame - visible_in), rowY.at(mouseover_row), true);
	}*/
}

bool KeyframeView::keyframeIsSelected(EffectField *field, int keyframe) {
	for (int i=0;i<selected_fields.size();i++) {
		if (selected_fields.at(i) == field && selected_keyframes.at(i) == keyframe) {
			return true;
		}
	}
	return false;
}

void KeyframeView::update_keys() {
//	panel_graph_editor->update_panel();
	update();
}

void KeyframeView::delete_selected_keyframes() {
	ComboAction* ca = new ComboAction();
	bool del = false;
	for (int i=0;i<selected_keyframes.size();i++) {
		// TODO these need to be sorted
		ca->append(new KeyframeDelete(selected_fields.at(i), selected_keyframes.at(i)));
		del = true;
	}
	if (del) {
		undo_stack.push(ca);

		selected_keyframes.clear();
		selected_fields.clear();
		update_keys();
		panel_sequence_viewer->viewer_widget->update();
	} else {
		delete ca;
	}
}

void KeyframeView::set_x_scroll(int s) {
	x_scroll = s;
	update_keys();
}

void KeyframeView::set_y_scroll(int s) {
	y_scroll = s;
	update_keys();
}

void KeyframeView::resize_move(double d) {
	header->update_zoom(header->get_zoom()*d);
}

void KeyframeView::mousePressEvent(QMouseEvent *event) {
	rect_select_x = event->x();
	rect_select_y = event->y();

	if (panel_timeline->tool == TIMELINE_TOOL_HAND || event->buttons() & Qt::MiddleButton) {
		scroll_drag = true;
		return;
	}

	old_key_vals.clear();

	int mouse_x = event->x() + x_scroll;
	int mouse_y = event->y();
	int row_index = -1;
	int field_index = -1;
	int keyframe_index = -1;
	long frame_diff = 0;
	long frame_min = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x-KEYFRAME_SIZE);
	drag_frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x);
	long frame_max = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x+KEYFRAME_SIZE);
	for (int i=0;i<rowY.size();i++) {
		if (mouse_y > rowY.at(i)-KEYFRAME_SIZE-KEYFRAME_SIZE && mouse_y < rowY.at(i)+KEYFRAME_SIZE+KEYFRAME_SIZE) {
			EffectRow* row = rows.at(i);

			row->focus_row();

			for (int k=0;k<row->fieldCount();k++) {
				EffectField* f = row->field(k);
				for (int j=0;j<f->keyframes.size();j++) {
					long eval_keyframe_time = f->keyframes.at(j).time-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in);
					if (eval_keyframe_time >= frame_min && eval_keyframe_time <= frame_max) {
						long eval_frame_diff = qAbs(eval_keyframe_time - drag_frame_start);
						if (keyframe_index == -1 || eval_frame_diff < frame_diff) {
							row_index = i;
							field_index = k;
							keyframe_index = j;
							frame_diff = eval_frame_diff;
						}
					}
				}
			}
			break;
		}
	}
	bool already_selected = false;
	keys_selected = false;
	if (keyframe_index > -1) already_selected = keyframeIsSelected(rows.at(row_index)->field(field_index), keyframe_index);
	if (!already_selected) {
		if (!(event->modifiers() & Qt::ShiftModifier)) {
			selected_fields.clear();
			selected_keyframes.clear();
		}
		if (keyframe_index > -1) {
			selected_fields.append(rows.at(row_index)->field(field_index));
			selected_keyframes.append(keyframe_index);

			// find other field with keyframes at the same time
			long comp_time = rows.at(row_index)->field(field_index)->keyframes.at(keyframe_index).time;
			for (int i=0;i<rows.at(row_index)->fieldCount();i++) {
				if (i != field_index) {
					EffectField* f = rows.at(row_index)->field(i);
					for (int j=0;j<f->keyframes.size();j++) {
						if (f->keyframes.at(j).time == comp_time) {
							selected_fields.append(f);
							selected_keyframes.append(j);
						}
					}
				}
			}
		}
	}

	if (selected_fields.size() > 0) {
		for (int i=0;i<selected_fields.size();i++) {
			old_key_vals.append(selected_fields.at(i)->keyframes.at(selected_keyframes.at(i)).time);
		}

		keys_selected = true;
	}

	update_keys();

	if (event->button() == Qt::LeftButton) {
		mousedown = true;
	}
}

void KeyframeView::mouseMoveEvent(QMouseEvent* event) {
	if (panel_timeline->tool == TIMELINE_TOOL_HAND) {
		setCursor(Qt::OpenHandCursor);
	} else {
		unsetCursor();
	}
	if (scroll_drag) {
		panel_effect_controls->horizontalScrollBar->setValue(panel_effect_controls->horizontalScrollBar->value() + rect_select_x - event->pos().x());
		panel_effect_controls->verticalScrollBar->setValue(panel_effect_controls->verticalScrollBar->value() + rect_select_y - event->pos().y());
		rect_select_x = event->pos().x();
		rect_select_y = event->pos().y();
	} else if (mousedown) {
		int mouse_x = event->x() + x_scroll;
		if (keys_selected) {
			// move keyframes
			long frame_diff = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x) - drag_frame_start;

			// snapping to playhead
			panel_timeline->snapped = false;
			if (panel_timeline->snapping) {
				for (int i=0;i<selected_keyframes.size();i++) {
					EffectField* field = selected_fields.at(i);
					Clip* c = field->parent_row->parent_effect->parent_clip;
					long key_time = old_key_vals.at(i) + frame_diff - c->clip_in + c->timeline_in;
					long key_eval = key_time;
					if (panel_timeline->snap_to_point(sequence->playhead, &key_eval)) {
						frame_diff += (key_eval - key_time);
						break;
					}
				}
			}

			// validate frame_diff (make sure no keyframes overlap each other)
			for (int i=0;i<selected_fields.size();i++) {
				EffectField* field = selected_fields.at(i);
				long eval_key = old_key_vals.at(i);
				for (int j=0;j<field->keyframes.size();j++) {
					while (!keyframeIsSelected(field, j) && field->keyframes.at(j).time == eval_key + frame_diff) {
						if (last_frame_diff > frame_diff) {
							frame_diff++;
							panel_timeline->snapped = false;
						} else {
							frame_diff--;
							panel_timeline->snapped = false;
						}
					}
				}
			}

			// apply frame_diffs
			for (int i=0;i<selected_keyframes.size();i++) {
				EffectField* field = selected_fields.at(i);
				field->keyframes[selected_keyframes.at(i)].time = old_key_vals.at(i) + frame_diff;
			}

			last_frame_diff = frame_diff;

			dragging = true;

			update_ui(false);
		} else {
			// do a rect select
			rect_select_w = event->x() - rect_select_x;
			rect_select_h = event->y() - rect_select_y;

			int min_row = qMin(rect_select_y, event->y())-KEYFRAME_SIZE;
			int max_row = qMax(rect_select_y, event->y())+KEYFRAME_SIZE;

			long frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, rect_select_x+x_scroll);
			long frame_end = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x);
			long min_frame = qMin(frame_start, frame_end)-KEYFRAME_SIZE;
			long max_frame = qMax(frame_start, frame_end)+KEYFRAME_SIZE;

			for (int i=0;i<rowY.size();i++) {
				if (rowY.at(i) >= min_row && rowY.at(i) <= max_row) {
					EffectRow* row = rows.at(i);
					for (int k=0;k<row->fieldCount();k++) {
						EffectField* field = row->field(k);
						for (int j=0;j<field->keyframes.size();j++) {
							long keyframe_frame = adjust_row_keyframe(row, field->keyframes.at(j).time);
							if (!keyframeIsSelected(field, j) && keyframe_frame >= min_frame && keyframe_frame <= max_frame) {
								selected_fields.append(field);
								selected_keyframes.append(j);
							}
						}
					}
				}
			}

			select_rect = true;

			update_keys();
		}
	}
}

void KeyframeView::mouseReleaseEvent(QMouseEvent*) {
	if (dragging) {
		ComboAction* ca = new ComboAction();
		for (int i=0;i<selected_fields.size();i++) {
			ca->append(new SetLong(
						   &selected_fields.at(i)->keyframes[selected_keyframes.at(i)].time,
						   old_key_vals.at(i),
						   selected_fields.at(i)->keyframes.at(selected_keyframes.at(i)).time
						));
		}
		undo_stack.push(ca);
	}

	select_rect = false;
	dragging = false;
	mousedown = false;
	scroll_drag = false;
	panel_timeline->snapped = false;
	update_ui(false);
}

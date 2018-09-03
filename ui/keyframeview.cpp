#include "keyframeview.h"

#include "effects/effect.h"
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

#include <QLabel>
#include <QMouseEvent>

#define KEYFRAME_SIZE 6
#define KEYFRAME_POINT_COUNT 4

long KeyframeView::adjust_row_keyframe(EffectRow* row, long time) {
	return time-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in);
}

KeyframeView::KeyframeView(QWidget *parent) : QWidget(parent), mousedown(false), dragging(false), keys_selected(false), select_rect(false), visible_in(0), visible_out(0) {
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
}

void KeyframeView::paintEvent(QPaintEvent*) {
	QPainter p(this);

	rowY.clear();
	rows.clear();

	long effects_in = LONG_MAX;
	long effects_out = 0;

	for (int j=0;j<panel_effect_controls->selected_clips.size();j++) {
		Clip* c = sequence->get_clip(panel_effect_controls->selected_clips.at(j));
		effects_in = qMin(effects_in, c->timeline_in);
		effects_out = qMax(effects_out, c->timeline_out);
		for (int i=0;i<c->effects.size();i++) {
			Effect* e = c->effects.at(i);
			if (e->container->is_expanded()) {
				for (int j=0;j<e->row_count();j++) {
					EffectRow* row = e->row(j);

					QLabel* label = row->label;
					QWidget* contents = e->container->contents;

					int keyframe_y = label->y() + (label->height()>>1) + mapFrom(panel_effect_controls, contents->mapTo(panel_effect_controls, contents->pos())).y() - e->container->title_bar->height();
					for (int k=0;k<row->keyframe_times.size();k++) {
						bool keyframe_selected = keyframeIsSelected(row, k);
						long keyframe_frame = adjust_row_keyframe(row, row->keyframe_times.at(k));
						if (dragging && keyframe_selected) keyframe_frame += frame_diff;
						draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, keyframe_frame), keyframe_y, keyframe_selected);
					}

					rows.append(row);
					rowY.append(keyframe_y);
				}
			}
		}
	}

	visible_in = effects_in;
	visible_out = effects_out;

	int width = getScreenPointFromFrame(panel_effect_controls->zoom, visible_out - visible_in);
	setMinimumWidth(width);
	header->setMinimumWidth(width);
	header->set_visible_in(effects_in);

	if (rowY.size() > 0) {
		int playhead_x = getScreenPointFromFrame(panel_effect_controls->zoom, panel_timeline->playhead-visible_in);
		p.setPen(Qt::red);
		p.drawLine(playhead_x, 0, playhead_x, height());
	}

	if (select_rect) {
		draw_selection_rectangle(p, QRect(rect_select_x, rect_select_y, rect_select_w, rect_select_h));
    }

    /*if (mouseover && mouseover_row < rowY.size()) {
		draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, mouseover_frame - visible_in), rowY.at(mouseover_row), true);
    }*/
}

bool KeyframeView::keyframeIsSelected(EffectRow *row, int keyframe) {
	for (int i=0;i<selected_rows.size();i++) {
		if (selected_rows.at(i) == row && selected_keyframes.at(i) == keyframe) {
			return true;
		}
	}
	return false;
}

void KeyframeView::delete_selected_keyframes() {
	KeyframeDelete* kd = new KeyframeDelete();
	bool del = false;
	for (int i=0;i<selected_keyframes.size();i++) {		
		selected_rows.at(i)->delete_keyframe(kd, selected_keyframes.at(i));
		del = true;
	}
	if (del) {
		undo_stack.push(kd);

		selected_keyframes.clear();
		selected_rows.clear();
		update();
		panel_viewer->viewer_widget->update();
	} else {
		delete kd;
	}
}

void KeyframeView::draw_keyframe(QPainter &p, int x, int y, bool darker) {
	QPoint points[KEYFRAME_POINT_COUNT] = {QPoint(x-KEYFRAME_SIZE, y), QPoint(x, y-KEYFRAME_SIZE), QPoint(x+KEYFRAME_SIZE, y), QPoint(x, y+KEYFRAME_SIZE)};
    int color = (darker) ? 100 : 160;
    p.setPen(QColor(0, 0, 0));
    p.setBrush(QColor(color, color, color));
	p.drawPolygon(points, KEYFRAME_POINT_COUNT);
}

void KeyframeView::mousePressEvent(QMouseEvent *event) {
    int row_index = -1;
    int keyframe_index = -1;
    long frame_diff = 0;
    long frame_min = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x()-KEYFRAME_SIZE);
	drag_frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x());
    long frame_max = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x()+KEYFRAME_SIZE);
    for (int i=0;i<rowY.size();i++) {
        if (event->y() > rowY.at(i)-KEYFRAME_SIZE-KEYFRAME_SIZE && event->y() < rowY.at(i)+KEYFRAME_SIZE+KEYFRAME_SIZE) {
            EffectRow* row = rows.at(i);
            for (int j=0;j<row->keyframe_times.size();j++) {
                long eval_keyframe_time = row->keyframe_times.at(j)-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in);
                if (eval_keyframe_time >= frame_min && eval_keyframe_time <= frame_max) {
					long eval_frame_diff = qAbs(eval_keyframe_time - drag_frame_start);
                    if (keyframe_index == -1 || eval_frame_diff < frame_diff) {
                        row_index = i;
                        keyframe_index = j;
                        frame_diff = eval_frame_diff;
                    }
                }
            }
            break;
        }
    }
    bool already_selected = false;
	keys_selected = false;
	if (keyframe_index > -1) already_selected = keyframeIsSelected(rows.at(row_index), keyframe_index);
    if (!already_selected) {
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            selected_rows.clear();
            selected_keyframes.clear();
        }
        if (keyframe_index > -1) {
			selected_rows.append(rows.at(row_index));
            selected_keyframes.append(keyframe_index);
        }
    }

	if (selected_rows.size() > 0) {
		keys_selected = true;
	} else {
		rect_select_x = event->x();
		rect_select_y = event->y();
	}

    update();
    mousedown = true;
}

void KeyframeView::mouseMoveEvent(QMouseEvent* event) {
    /*unsetCursor();
	bool new_mo = false;
	for (int i=0;i<rowY.size();i++) {
		if (event->y() > rowY.at(i)-KEYFRAME_SIZE-KEYFRAME_SIZE && event->y() < rowY.at(i)+KEYFRAME_SIZE+KEYFRAME_SIZE) {
			setCursor(Qt::CrossCursor);
			mouseover_frame = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x()) + visible_in;
			mouseover_row = i;
			new_mo = true;
			break;
		}
	}
	if (new_mo || (new_mo != mouseover)) {
		update();
	}
    mouseover = new_mo;*/
	if (mousedown) {
		if (keys_selected) {
			frame_diff = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x()) - drag_frame_start;

			// validate frame_diff
			for (int i=0;i<selected_keyframes.size();i++) {
				EffectRow* row = selected_rows.at(i);
				long eval_key = row->keyframe_times.at(selected_keyframes.at(i));
				for (int j=0;j<row->keyframe_times.size();j++) {
					while (!keyframeIsSelected(row, j) && row->keyframe_times.at(j) == eval_key + frame_diff) {
						if (last_frame_diff > frame_diff) {
							frame_diff++;
						} else {
							frame_diff--;
						}
					}
				}
			}

			last_frame_diff = frame_diff;

			dragging = true;
		} else {
			rect_select_w = event->x() - rect_select_x;
			rect_select_h = event->y() - rect_select_y;

			int min_row = qMin(rect_select_y, event->y())-KEYFRAME_SIZE;
			int max_row = qMax(rect_select_y, event->y())+KEYFRAME_SIZE;

			long frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, rect_select_x);
			long frame_end = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x());
			long min_frame = qMin(frame_start, frame_end)-KEYFRAME_SIZE;
			long max_frame = qMax(frame_start, frame_end)+KEYFRAME_SIZE;

			for (int i=0;i<rowY.size();i++) {
				if (rowY.at(i) >= min_row && rowY.at(i) <= max_row) {
					EffectRow* row = rows.at(i);
					for (int j=0;j<row->keyframe_times.size();j++) {
						long keyframe_frame = adjust_row_keyframe(row, row->keyframe_times.at(j));
						if (!keyframeIsSelected(row, j) && keyframe_frame >= min_frame && keyframe_frame <= max_frame) {
							selected_rows.append(rows.at(i));
							selected_keyframes.append(j);
						}
					}
				}
			}

			select_rect = true;
		}
		update();
    }
}

void KeyframeView::mouseReleaseEvent(QMouseEvent*) {
	if (dragging && frame_diff != 0) {
		KeyframeMove* ka = new KeyframeMove();
		ka->movement = frame_diff;
		ka->keyframes = selected_keyframes;
		ka->rows = selected_rows;
		undo_stack.push(ka);
	}

	select_rect = false;
	dragging = false;
    mousedown = false;
	update();
}

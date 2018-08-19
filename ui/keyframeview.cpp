#include "keyframeview.h"

#include "effects/effect.h"
#include "ui/collapsiblewidget.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "project/clip.h"
#include "panels/timeline.h"

#include <QLabel>
#include <QMouseEvent>

#define KEYFRAME_SIZE 6
#define KEYFRAME_POINT_COUNT 4

KeyframeView::KeyframeView(QWidget *parent) : QWidget(parent), mousedown(false), visible_in(0), visible_out(0) {
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
}

void KeyframeView::paintEvent(QPaintEvent*) {
	QPainter p(this);

	setMinimumWidth(getScreenPointFromFrame(panel_effect_controls->zoom, visible_out - visible_in));

	rowY.clear();
	rows.clear();
	for (int i=0;i<effects.size();i++) {
		Effect* e = effects.at(i);
		if (e->container->is_expanded()) {
			for (int j=0;j<e->row_count();j++) {
                EffectRow* row = e->row(j);

                QLabel* label = row->label;
				QWidget* contents = e->container->contents;

                int keyframe_y = label->y() + (label->height()>>1) + mapFrom(panel_effect_controls, contents->mapTo(panel_effect_controls, contents->pos())).y() - e->container->title_bar->height();
                for (int k=0;k<row->keyframe_times.size();k++) {
                    bool keyframe_selected = false;
                    for (int l=0;l<selected_rows.size();l++) {
//                        qDebug() << selected_rows.at(l) << rows.size() << selected_keyframes.at(l) << k;
                        if (selected_rows.at(l) == rows.size() && selected_keyframes.at(l) == k) {
                            keyframe_selected = true;
                            break;
                        }
                    }

                    draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, row->keyframe_times.at(k)-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in)), keyframe_y, keyframe_selected);
                }

                rows.append(row);
				rowY.append(keyframe_y);
			}
		}
	}

	if (rowY.size() > 0) {
		int playhead_x = getScreenPointFromFrame(panel_effect_controls->zoom, panel_timeline->playhead-visible_in);
		p.setPen(Qt::red);
		p.drawLine(playhead_x, 0, playhead_x, height());
	}

    /*if (mouseover && mouseover_row < rowY.size()) {
		draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, mouseover_frame - visible_in), rowY.at(mouseover_row), true);
    }*/
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
    long frame_mid = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x());
    long frame_max = getFrameFromScreenPoint(panel_effect_controls->zoom, event->x()+KEYFRAME_SIZE);
    for (int i=0;i<rowY.size();i++) {
        if (event->y() > rowY.at(i)-KEYFRAME_SIZE-KEYFRAME_SIZE && event->y() < rowY.at(i)+KEYFRAME_SIZE+KEYFRAME_SIZE) {
            EffectRow* row = rows.at(i);
            for (int j=0;j<row->keyframe_times.size();j++) {
                long eval_keyframe_time = row->keyframe_times.at(j)-row->parent_effect->parent_clip->clip_in+(row->parent_effect->parent_clip->timeline_in-visible_in);
                if (eval_keyframe_time >= frame_min && eval_keyframe_time <= frame_max) {
                    long eval_frame_diff = qAbs(eval_keyframe_time - frame_mid);
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
    if (keyframe_index > -1) {
        for (int i=0;i<selected_rows.size();i++) {
            if (selected_rows.at(i) == row_index && selected_keyframes.at(i) == keyframe_index) {
                already_selected = true;
            }
        }
    }
    if (!already_selected) {
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            selected_rows.clear();
            selected_keyframes.clear();
        }
        if (keyframe_index > -1) {
            selected_rows.append(row_index);
            selected_keyframes.append(keyframe_index);
        }
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

    }
}

void KeyframeView::mouseReleaseEvent(QMouseEvent* event) {
    mousedown = false;
}

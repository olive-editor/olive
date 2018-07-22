#include "timelineheader.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/undo.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#define CLICK_RANGE 5
#define PLAYHEAD_SIZE 6

TimelineHeader::TimelineHeader(QWidget *parent) : QWidget(parent), dragging(false), resizing_workarea(false) {
    setCursor(Qt::ArrowCursor);
    setMouseTracking(true);
}

void set_playhead(int mouse_x) {
    long frame = panel_timeline->getFrameFromScreenPoint(mouse_x);
    panel_timeline->snap_to_clip(&frame, false);
    panel_timeline->seek(frame);
    panel_timeline->repaint_timeline();
}

void TimelineHeader::set_in_point(long new_in) {
    long new_out = sequence->workarea_out;
    if (new_out == new_in) {
        new_in--;
    } else if (new_out < new_in) {
        new_out = sequence->getEndFrame();
    }

    TimelineAction* ta = new TimelineAction();
    ta->set_in_out(sequence, true, new_in, new_out);
    undo_stack.push(ta);

    update();
}

void TimelineHeader::set_out_point(long new_out) {
    long new_in = sequence->workarea_in;
    if (new_out == new_in) {
        new_out++;
    } else if (new_in > new_out) {
        new_in = 0;
    }

    TimelineAction* ta = new TimelineAction();
    ta->set_in_out(sequence, true, new_in, new_out);
    undo_stack.push(ta);

    update();
}

void TimelineHeader::mousePressEvent(QMouseEvent* event) {
    if (resizing_workarea) {
        sequence_end = sequence->getEndFrame();
    } else {
        set_playhead(event->pos().x());
    }
    dragging = true;
}

void TimelineHeader::mouseMoveEvent(QMouseEvent* event) {
    if (dragging) {
        if (resizing_workarea) {
            long frame = panel_timeline->getFrameFromScreenPoint(event->pos().x());
            panel_timeline->snap_to_clip(&frame, true);
            if (resizing_workarea_in) {
                temp_workarea_in = qMax(qMin(temp_workarea_out-1, frame), 0L);
            } else {
                temp_workarea_out = qMin(qMax(temp_workarea_in+1, frame), sequence_end);
            }
            panel_timeline->repaint_timeline();
        } else {
            set_playhead(event->pos().x());
        }
    } else {
        resizing_workarea = false;
        unsetCursor();
        if (sequence->using_workarea) {
            long min_frame = panel_timeline->getFrameFromScreenPoint(event->pos().x() - CLICK_RANGE) - 1;
            long max_frame = panel_timeline->getFrameFromScreenPoint(event->pos().x() + CLICK_RANGE) + 1;
            if (sequence->workarea_in > min_frame && sequence->workarea_in < max_frame) {
                resizing_workarea = true;
                resizing_workarea_in = true;
            } else if (sequence->workarea_out > min_frame && sequence->workarea_out < max_frame) {
                resizing_workarea = true;
                resizing_workarea_in = false;
            }
            if (resizing_workarea) {
                temp_workarea_in = sequence->workarea_in;
                temp_workarea_out = sequence->workarea_out;
                setCursor(Qt::SizeHorCursor);
            }
        }
    }
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent*) {
    if (resizing_workarea) {
        TimelineAction* ta = new TimelineAction();
        ta->set_in_out(sequence, true, temp_workarea_in, temp_workarea_out);
        undo_stack.push(ta);
    }

    resizing_workarea = false;
    dragging = false;
    panel_timeline->snapped = false;
    panel_timeline->repaint_timeline();
}

void TimelineHeader::paintEvent(QPaintEvent*) {
    if (sequence != NULL) {
        QPainter p(this);
        p.setPen(Qt::gray);
        int interval = 0;
        int multiplier = 0;
        do {
            multiplier++;
            interval = panel_timeline->getScreenPointFromFrame(sequence->frame_rate*multiplier);
        } while (interval < 10);
        for (int i=0;i<width();i+=interval) {
            p.drawLine(i, 0, i, height());
        }

        // draw in/out selection
        int in_x;
        if (sequence->using_workarea) {
            in_x = panel_timeline->getScreenPointFromFrame(resizing_workarea ? temp_workarea_in : sequence->workarea_in);
            int out_x = panel_timeline->getScreenPointFromFrame(resizing_workarea ? temp_workarea_out :sequence->workarea_out);
            p.fillRect(QRect(in_x, 0, out_x-in_x, height()), QColor(0, 192, 255, 128));
            p.setPen(Qt::white);
            p.drawLine(in_x, 0, in_x, height());
            p.drawLine(out_x, 0, out_x, height());
        }

        // draw playhead triangle
        in_x = panel_timeline->getScreenPointFromFrame(panel_timeline->playhead);
        QPoint start(in_x, height());
        QPainterPath path;
        path.moveTo(start);
        path.lineTo(in_x-PLAYHEAD_SIZE, 0);
        path.lineTo(in_x+PLAYHEAD_SIZE, 0);
        path.lineTo(start);
        p.fillPath(path, Qt::red);
    }
}

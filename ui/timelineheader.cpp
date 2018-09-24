#include "timelineheader.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "panels/viewer.h"
#include "io/config.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QScrollArea>
#include <QtMath>

#define CLICK_RANGE 5
#define PLAYHEAD_SIZE 6
#define LINE_MIN_PADDING 50
#define MARKER_SIZE 4

TimelineHeader::TimelineHeader(QWidget *parent) : QWidget(parent), dragging(false), resizing_workarea(false), zoom(1), in_visible(0), snapping(true), fm(font()) {
    setCursor(Qt::ArrowCursor);
    setMouseTracking(true);
    setMinimumHeight(fm.height()*2);
}

void TimelineHeader::set_playhead(int mouse_x) {
	long frame = getFrameFromScreenPoint(zoom, mouse_x) + in_visible;
	if (snapping) panel_timeline->snap_to_clip(&frame, false);
	panel_timeline->seek(frame);
}

void TimelineHeader::set_visible_in(long i) {
	in_visible = i;
	update();
}

void TimelineHeader::set_in_point(long new_in) {
    long new_out = sequence->workarea_out;
    if (new_out == new_in) {
        new_in--;
    } else if (new_out < new_in) {
        new_out = sequence->getEndFrame();
    }

    undo_stack.push(new SetTimelineInOutCommand(sequence, true, new_in, new_out));

    update();
}

void TimelineHeader::set_out_point(long new_out) {
    long new_in = sequence->workarea_in;
    if (new_out == new_in) {
        new_out++;
    } else if (new_in > new_out) {
        new_in = 0;
    }

    undo_stack.push(new SetTimelineInOutCommand(sequence, true, new_in, new_out));

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
			long frame = getFrameFromScreenPoint(zoom, event->pos().x());
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
			long min_frame = getFrameFromScreenPoint(zoom, event->pos().x() - CLICK_RANGE) - 1;
			long max_frame = getFrameFromScreenPoint(zoom, event->pos().x() + CLICK_RANGE) + 1;
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
        undo_stack.push(new SetTimelineInOutCommand(sequence, true, temp_workarea_in, temp_workarea_out));
    }

    resizing_workarea = false;
    dragging = false;
    panel_timeline->snapped = false;
    panel_timeline->repaint_timeline();
}

void TimelineHeader::update_header(double z) {
	zoom = z;
	update();
}

void TimelineHeader::paintEvent(QPaintEvent*) {
    if (sequence != NULL) {
        QPainter p(this);
        int yoff = height()/2;

        double interval = sequence->frame_rate;
        int textWidth = 0;
        int lastTextBoundary = 0;

        int i = 0;
        int lastLineX = -LINE_MIN_PADDING-1;

        int sublineCount = 1;
        int sublineTest = qRound(interval*zoom);
        int sublineInterval = 1;
        while (sublineTest > LINE_MIN_PADDING
               && sublineInterval >= 1) {
            sublineCount *= 2;
            sublineInterval = (interval/sublineCount);
            sublineTest = qRound(sublineInterval*zoom);
        }
        sublineCount = qMin(sublineCount, qRound(interval));

        while (true) {
            long frame = qRound(interval*i);
            int lineX = qRound(frame*zoom);
            if (lineX > width()) break;
            if (lineX > lastLineX+LINE_MIN_PADDING) {
                // draw text
                if (lineX-textWidth > lastTextBoundary) {
                    p.setPen(Qt::white);
					QString timecode = frame_to_timecode(frame + in_visible, config.timecode_view, sequence->frame_rate);
                    textWidth = fm.width(timecode)>>1;
                    lastTextBoundary = lineX+textWidth;
                    p.drawText(QRect(lineX-textWidth, 0, lastTextBoundary, yoff), timecode);
                }

                // draw line markers
                p.setPen(Qt::gray);
                p.drawLine(lineX, yoff, lineX, height());

                lastLineX = lineX;

                // draw sub-line markers
                for (int j=1;j<sublineCount;j++) {
                    int sublineX = lineX+(qRound(j*interval/sublineCount)*zoom);
                    p.drawLine(sublineX, yoff, sublineX, yoff+(yoff>>1));
                }
            }
            i++;
        }

        // draw in/out selection
        int in_x;
        if (sequence->using_workarea) {
			in_x = getScreenPointFromFrame(zoom, resizing_workarea ? temp_workarea_in : sequence->workarea_in);
			int out_x = getScreenPointFromFrame(zoom, resizing_workarea ? temp_workarea_out :sequence->workarea_out);
            p.fillRect(QRect(in_x, 0, out_x-in_x, height()), QColor(0, 192, 255, 128));
            p.setPen(Qt::white);
            p.drawLine(in_x, 0, in_x, height());
            p.drawLine(out_x, 0, out_x, height());
        }

		// draw markers
		for (int i=0;i<sequence->markers.size();i++) {
			const Marker& m = sequence->markers.at(i);
			int marker_x = getScreenPointFromFrame(zoom, m.frame);
			const QPoint points[5] = {
				QPoint(marker_x, height()-1),
				QPoint(marker_x + MARKER_SIZE, height() - MARKER_SIZE - 1),
				QPoint(marker_x + MARKER_SIZE, yoff),
				QPoint(marker_x - MARKER_SIZE, yoff),
				QPoint(marker_x - MARKER_SIZE, height() - MARKER_SIZE - 1)
			};
			p.setPen(Qt::black);
			p.setBrush(QColor(128, 224, 128));
			p.drawPolygon(points, 5);
		}

        // draw playhead triangle
		in_x = getScreenPointFromFrame(zoom, sequence->playhead - in_visible);
        QPoint start(in_x, height()+2);
        QPainterPath path;
        path.moveTo(start);
        path.lineTo(in_x-PLAYHEAD_SIZE, yoff);
        path.lineTo(in_x+PLAYHEAD_SIZE, yoff);
        path.lineTo(start);
        p.fillPath(path, Qt::red);
    }
}

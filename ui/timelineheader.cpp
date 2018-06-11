#include "timelineheader.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "project/sequence.h"

#include <QPainter>
#include <QMouseEvent>

#define PLAYHEAD_SIZE 6

TimelineHeader::TimelineHeader(QWidget *parent) : QWidget(parent)
{
    dragging = false;
}

void set_playhead(QMouseEvent* event) {
    long frame = panel_timeline->getFrameFromScreenPoint(event->pos().x());
    if (panel_timeline->snapping) {
        panel_timeline->snap_to_clip(&frame);
    }
    panel_timeline->seek(frame);
    panel_timeline->repaint_timeline();
}

void TimelineHeader::mousePressEvent(QMouseEvent* event) {
    set_playhead(event);
    dragging = true;
}

void TimelineHeader::mouseMoveEvent(QMouseEvent* event) {
    if (dragging) set_playhead(event);
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent *) {
    dragging = false;
}

void TimelineHeader::paintEvent(QPaintEvent*) {
    if (sequence != NULL) {
        QPainter p(this);
        p.setPen(Qt::gray);
        int interval = panel_timeline->getScreenPointFromFrame(sequence->frame_rate);
        for (int i=0;i<width();i+=interval) {
            p.drawLine(i, 0, i, height());
        }

        // draw playhead triangle
        int px = panel_timeline->getScreenPointFromFrame(panel_timeline->playhead);
        QPoint start(px, height());
        QPainterPath path;
        path.moveTo(start);
        path.lineTo(px-PLAYHEAD_SIZE, 0);
        path.lineTo(px+PLAYHEAD_SIZE, 0);
        path.lineTo(start);
        p.fillPath(path, Qt::red);
    }
}

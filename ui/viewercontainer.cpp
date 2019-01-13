#include "viewercontainer.h"

#include <QWidget>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>

#include "viewerwidget.h"
#include "panels/viewer.h"
#include "project/sequence.h"
#include "debug.h"

// enforces aspect ratio
ViewerContainer::ViewerContainer(QWidget *parent) :
    QScrollArea(parent),
    fit(true),
    child(nullptr)
{
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::NoFrame);

    area = new QWidget(this);
    area->move(0, 0);
    setWidget(area);

    child = new ViewerWidget(area);
    child->container = this;
}

ViewerContainer::~ViewerContainer() {
    delete area;
}

void ViewerContainer::dragScrollPress(const QPoint &p) {
    drag_start_x = p.x();
    drag_start_y = p.y();
    horiz_start = horizontalScrollBar()->value();
    vert_start = verticalScrollBar()->value();
}

void ViewerContainer::dragScrollMove(const QPoint &p) {
    int true_x = p.x() + (horiz_start - horizontalScrollBar()->value());
    int true_y = p.y() + (vert_start - verticalScrollBar()->value());

    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (drag_start_x - true_x));
    verticalScrollBar()->setValue(verticalScrollBar()->value() + (drag_start_y - true_y));

    drag_start_x = true_x;
    drag_start_y = true_y;
}

void ViewerContainer::adjust() {
    if (viewer->seq != nullptr) {
        if (child->waveform) {
            child->move(0, 0);
            child->resize(size());
        } else if (fit) {
            double aspect_ratio = double(viewer->seq->width)/double(viewer->seq->height);

            int widget_x = 0;
            int widget_y = 0;
            int widget_width = width();
            int widget_height = height();
            float widget_ar = (float) widget_width /(float) widget_height;

            bool widget_is_wider_than_sequence = widget_ar > aspect_ratio;

            if (widget_is_wider_than_sequence) {
                widget_width = widget_height * aspect_ratio;
                widget_x = (width() / 2) - (widget_width / 2);
            } else {
                widget_height = widget_width / aspect_ratio;
                widget_y = (height() / 2) - (widget_height / 2);
            }

            child->move(widget_x, widget_y);
            child->resize(widget_width, widget_height);

            zoom = double(widget_width) / double(viewer->seq->width);
        } else {
            int zoomed_width = double(viewer->seq->width)*zoom;
            int zoomed_height = double(viewer->seq->height)*zoom;
            int zoomed_x = 0;
            int zoomed_y = 0;

            if (zoomed_width < width()) zoomed_x = (width()>>1)-(zoomed_width>>1);
            if (zoomed_height < height()) zoomed_y = (height()>>1)-(zoomed_height>>1);

            child->move(zoomed_x, zoomed_y);
            child->resize(zoomed_width, zoomed_height);
        }
	}
    area->resize(qMax(width(), child->width()), qMax(height(), child->height()));
}

void ViewerContainer::resizeEvent(QResizeEvent *event) {
	event->accept();
	adjust();
}

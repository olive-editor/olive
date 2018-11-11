#include "viewercontainer.h"

#include <QWidget>
#include <QResizeEvent>

// enforces aspect ratio
ViewerContainer::ViewerContainer(QWidget *parent) :
	QWidget(parent),
    aspect_ratio(1),
    child(NULL)
{}

void ViewerContainer::adjust() {
	if (child != NULL) {
		QSize widget_size = size();
		int widget_x = 0;
		int widget_y = 0;
		int widget_width = widget_size.width();
		int widget_height = widget_size.height();
		float widget_ar = (float) widget_width /(float) widget_height;

		bool widget_is_larger_than_sequence = widget_ar > aspect_ratio;

		if (widget_is_larger_than_sequence) {
			widget_width = widget_height * aspect_ratio;
			widget_x = (widget_size.width() / 2) - (widget_width / 2);
		} else {
			widget_height = widget_width / aspect_ratio;
			widget_y = (widget_size.height() / 2) - (widget_height / 2);
		}

		child->move(widget_x, widget_y);
		child->resize(widget_width, widget_height);
	}
}

void ViewerContainer::resizeEvent(QResizeEvent *event) {
	event->accept();
	adjust();
}

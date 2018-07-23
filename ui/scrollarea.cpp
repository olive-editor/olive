#include "scrollarea.h"

#include <QWheelEvent>
#include <QDebug>

#include "io/config.h"
#include "panels/panels.h"
#include "panels/timeline.h"

ScrollArea::ScrollArea(QWidget* parent) : QScrollArea(parent) {}

void ScrollArea::wheelEvent(QWheelEvent *e) {
    if (config.scroll_zooms) {
        e->ignore();
        if (e->angleDelta().y() != 0) panel_timeline->set_zoom(e->angleDelta().y() > 0);
    } else {
        QScrollArea::wheelEvent(e);
    }
}

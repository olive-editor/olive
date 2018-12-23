#include "resizablescrollbar.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

#define RESIZE_HANDLE_SIZE 10

ResizableScrollBar::ResizableScrollBar(QWidget *parent) :
    QScrollBar(parent),
    resize_init(false),
    resize_proc(false)
{
    setMouseTracking(true);
}

bool ResizableScrollBar::is_resizing() {
    return resize_proc;
}

void ResizableScrollBar::mousePressEvent(QMouseEvent *e) {
    if (resize_init) {
        resize_proc = true;
        resize_start = e->pos().x();
    } else {
        QScrollBar::mousePressEvent(e);
    }
}

void ResizableScrollBar::mouseMoveEvent(QMouseEvent *e) {
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                           QStyle::SC_ScrollBarSlider, this);

    if (resize_proc) {
        int diff = e->pos().x() - resize_start;
        if (resize_top) {
            setValue(value() + diff);
            diff = -diff;
        }
        int w = sr.width() + diff;
        emit resized_scroll(double(sr.width())/double(w));
        resize_start = e->pos().x();
    } else {
        bool new_resize_init = false;

        if ((orientation() == Qt::Horizontal && e->pos().x() > sr.left()-RESIZE_HANDLE_SIZE && e->pos().x() < sr.left()+RESIZE_HANDLE_SIZE)
                || (orientation() == Qt::Vertical && e->pos().y() > sr.top()-RESIZE_HANDLE_SIZE && e->pos().y() < sr.top()+RESIZE_HANDLE_SIZE)) {
            new_resize_init = true;
            resize_top = true;
        } else if ((orientation() == Qt::Horizontal && e->pos().x() > sr.right()-RESIZE_HANDLE_SIZE && e->pos().x() < sr.right()+RESIZE_HANDLE_SIZE)
                   || (orientation() == Qt::Vertical && e->pos().y() > sr.bottom()-RESIZE_HANDLE_SIZE && e->pos().y() < sr.bottom()+RESIZE_HANDLE_SIZE)) {
            new_resize_init = true;
            resize_top = false;
        }

        if (resize_init != new_resize_init) {
            if (new_resize_init) {
                setCursor(Qt::SizeHorCursor);
            } else {
                unsetCursor();
            }
            resize_init = new_resize_init;
        }

        QScrollBar::mouseMoveEvent(e);
    }
}

void ResizableScrollBar::mouseReleaseEvent(QMouseEvent *e) {
    if (resize_proc) {
        resize_proc = false;
    } else {
        QScrollBar::mouseReleaseEvent(e);
    }
}

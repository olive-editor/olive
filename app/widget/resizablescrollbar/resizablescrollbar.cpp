#include "resizablescrollbar.h"

#include <QDebug>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSlider>

#include "common/range.h"

const int ResizableScrollBar::kHandleWidth = 10;

ResizableScrollBar::ResizableScrollBar(QWidget *parent) :
  QScrollBar(parent)
{
  Init();
}

ResizableScrollBar::ResizableScrollBar(Qt::Orientation orientation, QWidget *parent):
  QScrollBar(orientation, parent)
{
  Init();
}

void ResizableScrollBar::mousePressEvent(QMouseEvent *event)
{
  if (mouse_handle_state_ == kNotInHandle) {
    QScrollBar::mousePressEvent(event);
  } else {
    mouse_dragging_ = true;

    mouse_drag_start_ = GetActiveMousePos(event);
  }
}

void ResizableScrollBar::mouseMoveEvent(QMouseEvent *event)
{
  QStyleOptionSlider opt;
  initStyleOption(&opt);

  QRect sr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                     QStyle::SC_ScrollBarSlider, this);
  QRect gr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                     QStyle::SC_ScrollBarGroove, this);

  if (mouse_dragging_) {

    int new_drag_pos = GetActiveMousePos(event);
    int mouse_movement = new_drag_pos - mouse_drag_start_;
    mouse_drag_start_ = new_drag_pos;

    if (mouse_handle_state_ == kInTopHandle) {
      mouse_movement = -mouse_movement;
    }

    double scale_multiplier = static_cast<double>(sr.width()) / static_cast<double>(sr.width() + mouse_movement);
    emit RequestScale(scale_multiplier);

    if (mouse_handle_state_ == kInTopHandle) {
      int slider_min = gr.x();
      int slider_max = gr.right() - (sr.width() + mouse_movement);
      int val = QStyle::sliderValueFromPosition(minimum(),
                                                maximum(),
                                                event->pos().x() - slider_min,
                                                slider_max - slider_min,
                                                opt.upsideDown);

      setValue(val);
    } else {
      setValue(qRound(static_cast<double>(value()) * scale_multiplier));
    }

  } else {

    int mouse_pos, top, bottom;
    Qt::CursorShape target_cursor;
    mouse_pos = GetActiveMousePos(event);

    if (orientation() == Qt::Horizontal) {
      top = sr.left();
      bottom = sr.right();
      target_cursor = Qt::SizeHorCursor;
    } else {
      top = sr.top();
      bottom = sr.bottom();
      target_cursor = Qt::SizeVerCursor;
    }

    if (InRange(mouse_pos, top, kHandleWidth)) {
      mouse_handle_state_ = kInTopHandle;
    } else if (InRange(mouse_pos, bottom, kHandleWidth)) {
      mouse_handle_state_ = kInBottomHandle;
    } else {
      mouse_handle_state_ = kNotInHandle;
    }

    if (mouse_handle_state_ == kNotInHandle) {
      unsetCursor();
    } else {
      setCursor(target_cursor);
    }

    QScrollBar::mouseMoveEvent(event);

  }
}

void ResizableScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
  if (mouse_dragging_) {
    mouse_dragging_ = false;
  } else {
    QScrollBar::mouseReleaseEvent(event);
  }
}

void ResizableScrollBar::Init()
{
  setSingleStep(20);
  setMaximum(0);
  setMouseTracking(true);

  mouse_handle_state_= kNotInHandle;
  mouse_dragging_ = false;
}

int ResizableScrollBar::GetActiveMousePos(QMouseEvent *event)
{
  if (orientation() == Qt::Horizontal) {
    return event->pos().x();
  } else {
    return event->pos().y();
  }
}

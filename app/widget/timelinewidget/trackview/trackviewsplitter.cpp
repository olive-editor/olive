#include "trackviewsplitter.h"

#include <QDebug>
#include <QPainter>

#include "trackviewitem.h"

TrackViewSplitter::TrackViewSplitter(Qt::Alignment vertical_alignment, QWidget* parent) :
  QSplitter(Qt::Vertical, parent),
  alignment_(vertical_alignment)
{
  setHandleWidth(1);

  int initial_height = 0;

  if (alignment_ == Qt::AlignBottom) {
    // Add empty spacer so we get a splitter handle after the last element
    addWidget(new QWidget());
  }

  for (int i=0;i<3;i++) {
    TrackViewItem* w = new TrackViewItem();
    addWidget(w);

    initial_height += w->sizeHint().height() + handleWidth();
  }

  if (alignment_ == Qt::AlignTop) {
    // Add empty spacer so we get a splitter handle after the last element
    addWidget(new QWidget());
  }

  setFixedHeight(initial_height);
}

void TrackViewSplitter::HandleReceiver(TrackViewSplitterHandle *h, int diff)
{
  int ele_id = -1;

  for (int i=0;i<count();i++) {
    if (handle(i) == h) {
      ele_id = i;
      break;
    }
  }

  // The handle index is actually always one above the element index
  if (alignment_ == Qt::AlignTop) {
    ele_id--;
  } else if (alignment_ == Qt::AlignBottom) {
    diff = -diff;
  }

  QList<int> element_sizes = sizes();

  int old_ele_sz = element_sizes.at(ele_id);

  // Transform element size by diff
  int new_ele_sz = old_ele_sz + diff;

  // Validate it with the widget's minimum size
  new_ele_sz = qMax(new_ele_sz, widget(ele_id)->minimumHeight());

  // Correct diff
  diff = new_ele_sz - old_ele_sz;

  // Set new size on element
  element_sizes.replace(ele_id, new_ele_sz);
  setSizes(element_sizes);

  // Increase height by the difference
  setFixedHeight(height() + diff);
}

QSplitterHandle *TrackViewSplitter::createHandle()
{
  return new TrackViewSplitterHandle(orientation(), this);
}

TrackViewSplitterHandle::TrackViewSplitterHandle(Qt::Orientation orientation, QSplitter *parent) :
  QSplitterHandle(orientation, parent),
  dragging_(false)
{
}

void TrackViewSplitterHandle::mousePressEvent(QMouseEvent *)
{
}

void TrackViewSplitterHandle::mouseMoveEvent(QMouseEvent *)
{
  if (dragging_) {
    static_cast<TrackViewSplitter*>(parent())->HandleReceiver(this, QCursor::pos().y() - drag_y_);
  }

  drag_y_ = QCursor::pos().y();
  dragging_ = true;
}

void TrackViewSplitterHandle::mouseReleaseEvent(QMouseEvent *)
{
  dragging_ = false;
}

void TrackViewSplitterHandle::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.fillRect(rect(), palette().base());
}

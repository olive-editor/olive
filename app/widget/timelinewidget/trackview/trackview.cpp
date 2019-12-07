#include "trackview.h"

#include <QDebug>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

#include "trackviewitem.h"

TrackView::TrackView(Qt::Alignment vertical_alignment, QWidget *parent) :
  QScrollArea(parent),
  alignment_(vertical_alignment)
{
  QWidget* central = new QWidget();
  setWidget(central);
  setWidgetResizable(true);

  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setMargin(0);
  layout->setSpacing(0);

  if (alignment_ == Qt::AlignBottom) {
    layout->addStretch();

    connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(ScrollbarRangeChanged(int, int)));
    last_scrollbar_max_ = verticalScrollBar()->maximum();
  }

  splitter_ = new TrackViewSplitter(alignment_);
  splitter_->setChildrenCollapsible(false);
  layout->addWidget(splitter_);
}

void TrackView::resizeEvent(QResizeEvent *event)
{
  QScrollArea::resizeEvent(event);

  //splitter_->setFixedWidth(viewport()->width());
}

void TrackView::ScrollbarRangeChanged(int, int max)
{
  if (max != last_scrollbar_max_) {
    int ba_val = last_scrollbar_max_ - verticalScrollBar()->value();
    int new_val = max - ba_val;

    verticalScrollBar()->setValue(new_val);

    last_scrollbar_max_ = max;
  }
}

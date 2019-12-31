#include "timelineandtrackview.h"

#include <QHBoxLayout>
#include <QScrollBar>

TimelineAndTrackView::TimelineAndTrackView(const TrackType &type, Qt::Alignment vertical_alignment, QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  splitter_ = new QSplitter(Qt::Horizontal);
  splitter_->setChildrenCollapsible(false);
  layout->addWidget(splitter_);

  track_view_ = new TrackView(vertical_alignment);
  splitter_->addWidget(track_view_);

  view_ = new TimelineView(type, vertical_alignment);
  splitter_->addWidget(view_);

  connect(view_->verticalScrollBar(), &QScrollBar::valueChanged, this, &TimelineAndTrackView::ViewValueChanged);
  connect(track_view_->verticalScrollBar(), &QScrollBar::valueChanged, this, &TimelineAndTrackView::TracksValueChanged);

  splitter_->setSizes({1, width()});
}

QSplitter *TimelineAndTrackView::splitter() const
{
  return splitter_;
}

TimelineView *TimelineAndTrackView::view() const
{
  return view_;
}

TrackView *TimelineAndTrackView::track_view() const
{
  return track_view_;
}

void TimelineAndTrackView::ViewValueChanged(int v)
{
  track_view_->verticalScrollBar()->setValue(v - view_->verticalScrollBar()->minimum());
}

void TimelineAndTrackView::TracksValueChanged(int v)
{
  view_->verticalScrollBar()->setValue(view_->verticalScrollBar()->minimum() + v);
}

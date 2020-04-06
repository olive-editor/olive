/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timelineandtrackview.h"

#include <QHBoxLayout>
#include <QScrollBar>

OLIVE_NAMESPACE_ENTER

TimelineAndTrackView::TimelineAndTrackView(Qt::Alignment vertical_alignment, QWidget *parent) :
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

  view_ = new TimelineView(vertical_alignment);
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

OLIVE_NAMESPACE_EXIT

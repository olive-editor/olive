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

#include "trackview.h"

#include <QDebug>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

#include "trackviewitem.h"

OLIVE_NAMESPACE_ENTER

TrackView::TrackView(Qt::Alignment vertical_alignment, QWidget *parent) :
  QScrollArea(parent),
  list_(nullptr),
  alignment_(vertical_alignment)
{
  setAlignment(Qt::AlignLeft | alignment_);

  QWidget* central = new QWidget();
  setWidget(central);
  setWidgetResizable(true);

  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setMargin(0);
  layout->setSpacing(0);

  if (alignment_ == Qt::AlignBottom) {
    layout->addStretch();

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &TrackView::ScrollbarRangeChanged);
    last_scrollbar_max_ = verticalScrollBar()->maximum();
  }

  splitter_ = new TrackViewSplitter(alignment_);
  splitter_->setChildrenCollapsible(false);
  layout->addWidget(splitter_);

  if (alignment_ == Qt::AlignTop) {
    layout->addStretch();
  }

  connect(splitter_, &TrackViewSplitter::TrackHeightChanged, this, &TrackView::TrackHeightChanged);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void TrackView::ConnectTrackList(TrackList *list)
{
  if (list_ != nullptr) {
    foreach (TrackOutput* track, list_->GetTracks()) {
      RemoveTrack(track);
    }

    disconnect(list_, &TrackList::TrackHeightChanged, splitter_, &TrackViewSplitter::SetTrackHeight);
    disconnect(list_, &TrackList::TrackAdded, this, &TrackView::InsertTrack);
    disconnect(list_, &TrackList::TrackRemoved, this, &TrackView::RemoveTrack);
  }

  list_ = list;

  if (list_ != nullptr) {
    foreach (TrackOutput* track, list_->GetTracks()) {
      InsertTrack(track);
    }

    connect(list_, &TrackList::TrackHeightChanged, splitter_, &TrackViewSplitter::SetTrackHeight);
    connect(list_, &TrackList::TrackAdded, this, &TrackView::InsertTrack);
    connect(list_, &TrackList::TrackRemoved, this, &TrackView::RemoveTrack);
  }
}

void TrackView::DisconnectTrackList()
{
  ConnectTrackList(nullptr);
}

void TrackView::resizeEvent(QResizeEvent *e)
{
  QScrollArea::resizeEvent(e);

  splitter_->SetSpacerHeight(height()/2);
}

void TrackView::ScrollbarRangeChanged(int, int max)
{
  if (max != last_scrollbar_max_) {
    int ba_val = last_scrollbar_max_ - verticalScrollBar()->value();
    int new_val = max - ba_val;

    verticalScrollBar()->setValue(new_val);
    emit verticalScrollBar()->valueChanged(new_val);

    last_scrollbar_max_ = max;
  }
}

void TrackView::TrackHeightChanged(int index, int height)
{
  list_->GetTrackAt(index)->SetTrackHeightInPixels(height);
}

void TrackView::InsertTrack(TrackOutput *track)
{
  splitter_->Insert(track->Index(),
                    track->GetTrackHeightInPixels(),
                    new TrackViewItem(track));
}

void TrackView::RemoveTrack(TrackOutput *track)
{
  splitter_->Remove(track->Index());
}

OLIVE_NAMESPACE_EXIT

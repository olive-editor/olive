#include "trackview.h"

#include <QDebug>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

#include "trackviewitem.h"

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
    foreach (TrackViewItem* item, items_) {
      delete item;
    }
    items_.clear();

    disconnect(list_, &TrackList::TrackHeightChanged, splitter_, &TrackViewSplitter::SetTrackHeight);
    disconnect(list_, &TrackList::TrackAdded, this, &TrackView::InsertTrack);
    disconnect(list_, &TrackList::TrackRemoved, this, &TrackView::RemoveTrack);
  }

  list_ = list;

  if (list_ != nullptr) {
    foreach (TrackOutput* track, list_->Tracks()) {
      TrackViewItem* item = new TrackViewItem(track);
      items_.append(item);
      splitter_->Insert(track->Index(), track->GetTrackHeight(), item);
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
  list_->TrackAt(index)->SetTrackHeight(height);
}

void TrackView::InsertTrack(TrackOutput *track)
{
  splitter_->Insert(track->Index(), track->GetTrackHeight(), new TrackViewItem(track));
}

void TrackView::RemoveTrack(TrackOutput *track)
{
  splitter_->Remove(track->Index());
}

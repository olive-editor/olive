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

    connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(ScrollbarRangeChanged(int, int)));
    last_scrollbar_max_ = verticalScrollBar()->maximum();
  }

  splitter_ = new TrackViewSplitter(alignment_);
  splitter_->setChildrenCollapsible(false);
  layout->addWidget(splitter_);

  if (alignment_ == Qt::AlignTop) {
    layout->addStretch();
  }

  connect(splitter_, SIGNAL(TrackHeightChanged(int, int)), this, SLOT(TrackHeightChanged(int, int)));
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

    disconnect(list_, SIGNAL(TrackHeightChanged(int, int)), splitter_, SLOT(SetTrackHeight(int, int)));
    disconnect(list_, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(InsertTrack(TrackOutput*)));
    disconnect(list_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
  }

  list_ = list;

  if (list_ != nullptr) {
    foreach (TrackOutput* track, list_->Tracks()) {
      TrackViewItem* item = new TrackViewItem(track);
      items_.append(item);
      splitter_->Insert(track->Index(), track->GetTrackHeight(), item);
    }

    connect(list_, SIGNAL(TrackHeightChanged(int, int)), splitter_, SLOT(SetTrackHeight(int, int)));
    connect(list_, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(InsertTrack(TrackOutput*)));
    connect(list_, SIGNAL(TrackRemoved(TrackOutput*)), this, SLOT(RemoveTrack(TrackOutput*)));
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

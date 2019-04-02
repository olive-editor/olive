#include "timelinearea.h"

TimelineArea::TimelineArea(Timeline* timeline) :
  timeline_(timeline),
  track_list_(nullptr),
  alignment_(olive::timeline::kAlignmentTop)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  // LABELS
  QWidget* label_container = new QWidget();
  label_container_layout_ = new QVBoxLayout(label_container);
  layout->addWidget(label_container);

  // VIEW
  view_ = new TimelineView(timeline_);
  layout->addWidget(view_);

  // SCROLLBAR
  QScrollBar* scrollbar = new QScrollBar(Qt::Vertical);
  layout->addWidget(scrollbar);

  view_->scrollBar = scrollbar;
}

void TimelineArea::SetAlignment(olive::timeline::Alignment alignment)
{
  alignment_ = alignment;
  view_->SetAlignment(alignment);
}

void TimelineArea::SetTrackList(Sequence *sequence, Track::Type track_list)
{
  if (sequence == nullptr) {

    track_list_ = nullptr;

  } else {

    track_list_ = sequence->GetTrackList(track_list);

  }

  view_->SetTrackList(track_list_);
}

void TimelineArea::RefreshLabels()
{
  if (track_list_ == nullptr) {
    labels_.clear();
  } else {

    labels_.resize(track_list_->TrackCount());
    for (int i=0;i<labels_.size();i++) {
      labels_[i]->SetTrack(track_list_->TrackAt(i));
    }

  }
}

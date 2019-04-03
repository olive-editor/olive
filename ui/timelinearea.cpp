#include "timelinearea.h"

int olive::timeline::kTimelineLabelFixedWidth = 200;

TimelineArea::TimelineArea(Timeline* timeline) :
  timeline_(timeline),
  track_list_(nullptr),
  alignment_(olive::timeline::kAlignmentTop)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  // LABELS
  QWidget* label_container = new QWidget();
  label_container->setFixedWidth(olive::timeline::kTimelineLabelFixedWidth);
  label_container_layout_ = new QVBoxLayout(label_container);
  label_container_layout_->setMargin(0);
  label_container_layout_->setSpacing(0);
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

    labels_.clear();

  } else {

    track_list_ = sequence->GetTrackList(track_list);

    labels_.resize(track_list_->TrackCount());
    for (int i=0;i<labels_.size();i++) {
      labels_[i] = std::make_shared<TimelineLabel>();
      labels_[i]->SetTrack(track_list_->TrackAt(i));
      label_container_layout_->addWidget(labels_[i].get());
    }

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

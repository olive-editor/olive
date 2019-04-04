#include "timelinearea.h"

int olive::timeline::kTimelineLabelFixedWidth = 200;

TimelineArea::TimelineArea(Timeline* timeline, olive::timeline::Alignment alignment) :
  timeline_(timeline),
  track_list_(nullptr),
  alignment_(alignment)
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
  label_container_layout_->addStretch();
  layout->addWidget(label_container);

  // VIEW
  view_ = new TimelineView(timeline_);
  view_->SetAlignment(alignment_);
  layout->addWidget(view_);

  // SCROLLBAR
  QScrollBar* scrollbar = new QScrollBar(Qt::Vertical);
  layout->addWidget(scrollbar);

  view_->scrollBar = scrollbar;
}

void TimelineArea::SetTrackList(Sequence *sequence, Track::Type track_list)
{
  if (track_list_ != nullptr) {
    disconnect(track_list_, SIGNAL(TrackCountChanged()), this, SLOT(RefreshLabels()));
  }

  if (sequence == nullptr) {

    track_list_ = nullptr;

  } else {

    track_list_ = sequence->GetTrackList(track_list);
    connect(track_list_, SIGNAL(TrackCountChanged()), this, SLOT(RefreshLabels()));

  }

  RefreshLabels();

  view_->SetTrackList(track_list_);

}

void TimelineArea::RefreshLabels()
{
  if (track_list_ == nullptr) {

    labels_.clear();

  } else {

    labels_.resize(track_list_->TrackCount());
    for (int i=0;i<labels_.size();i++) {
      labels_[i] = std::make_shared<TimelineLabel>();
      labels_[i]->SetTrack(track_list_->TrackAt(i));

      label_container_layout_->insertWidget(label_container_layout_->count()-1, labels_[i].get());
    }

  }
}

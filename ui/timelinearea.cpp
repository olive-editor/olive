#include "timelinearea.h"

TimelineArea::TimelineArea() :
  track_list_(nullptr)
{

}

void TimelineArea::SetTrackList(Sequence *sequence, Track::Type track_list)
{
  if (sequence == nullptr) {

    track_list_ = nullptr;

  } else {

    track_list_ = sequence->GetTrackList(track_list);

  }


}

void TimelineArea::RefreshLabels()
{
  if (track_list_ == nullptr) {
    labels_.clear();
  } else {

    labels_.resize(track_list_->TrackCount());
    for (int i=0;i<labels_.size();i++) {
      labels_.at(i).settra
    }

  }
}

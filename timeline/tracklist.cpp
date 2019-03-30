#include "tracklist.h"

TrackList::TrackList(Sequence *parent, Track::Type type) :
  QObject(parent),
  type_(type)
{
  // Ensure we have at least one track
  AddTrack();
}

TrackListPtr TrackList::copy(Sequence *parent)
{
  TrackListPtr t = std::make_shared<TrackList>(parent, type_);

  t->ResizeTrackArray(tracks_.size());
  for (int i=0;i<tracks_.size();i++) {
    t->tracks_[i] = tracks_.at(i)->copy(t.get());
  }

  return t;
}

void TrackList::AddTrack()
{
  TrackPtr track = std::make_shared<Track>(this, type_);
  tracks_.append(track);
}

void TrackList::RemoveTrack(int i)
{
  if (tracks_.size() == 1) {
    return;
  }
  tracks_.removeAt(i);
}

Track *TrackList::First()
{
  return tracks_.first().get();
}

int TrackList::TrackCount()
{
  return tracks_.size();
}

QVector<TrackPtr> TrackList::tracks()
{
  return tracks_;
}

Sequence *TrackList::GetParent()
{
  return static_cast<Sequence*>(parent());
}

#include "project/sequence/sequence.h"

namespace olive {

Sequence::Sequence() {

}

Track* const Sequence::addTrack() {
  return doAddTrack();
}

Track* const Sequence::doAddTrack() {
  // Stack allocation would be enough but leave it now
  Track* track = new Track();
  tracks_.emplace_back(track);
  onDidAddTrack(track, tracks_.size() - 1);
  return track;
}

void Sequence::removeTrackAt(int index) {
  doRemoveTrackAt(index);
}

void Sequence::doRemoveTrackAt(int index) {
  if (index < 0 || tracks_.size() >= index) return;
  Track* track = tracks_[index];
  onWillRemoveTrack(track, index);
  tracks_.erase(tracks_.begin() + index);
}

Track* const Sequence::getTrackAt(int index) {
  if (index < 0 || tracks_.size() >= index) return nullptr;
  return tracks_[index];
}

void Sequence::setDuration(int value) {
  duration_ = value;
}

int Sequence::duration() const {
  return duration_;
}

const std::vector<Track*>& Sequence::tracks() {
  return tracks_;
}

int Sequence::track_size() const {
  return tracks_.size();
}

}
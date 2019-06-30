#include "project/sequence/track.h"

namespace olive {

Track::Track() {

}

void Track::addClip(Clip* const clip) {
  doAddClip(clip);
}

void Track::doAddClip(Clip* const clip) {
  if (hasClip(clip)) return;
  clips_.insert(clip);
  onDidAddClip(clip);
}

void Track::removeClip(Clip* const clip) {
  doRemoveClip(clip);
}

void Track::doRemoveClip(Clip* const clip) {
  if (!hasClip(clip)) return;
  onWillRemoveClip(clip);
  clips_.erase(clip);
}

bool Track::hasClip(Clip* const clip) const {
  return clips_.count(clip) > 0;
}

const std::set<Clip*>& Track::clips() {
  return clips_;
}

}
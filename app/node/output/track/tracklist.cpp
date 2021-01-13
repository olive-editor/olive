/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "tracklist.h"

#include "node/factory.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/output/viewer/viewer.h"

namespace olive {

TrackList::TrackList(ViewerOutput *parent, const Track::Type &type, NodeInput *track_input) :
  QObject(parent),
  track_input_(track_input),
  type_(type)
{
  connect(track_input_, &NodeInput::InputConnected, this, &TrackList::TrackConnected);
  connect(track_input_, &NodeInput::InputDisconnected, this, &TrackList::TrackDisconnected);
}

Track *TrackList::GetTrackAt(int index) const
{
  if (index < track_cache_.size()) {
    return track_cache_.at(index);
  } else {
    return nullptr;
  }
}

void TrackList::TrackConnected(Node *node, int element)
{
  if (element == -1) {
    // FIXME: Should probably merge into Viewer so we can actually do this
    //static_cast<ViewerOutput*>(parent())->InputConnectionChanged(node, element);
    return;
  }

  Track* track = dynamic_cast<Track*>(node);

  if (!track) {
    return;
  }

  // Find "real" index
  Track* next = nullptr;
  for (int i=element+1; i<track_input_->ArraySize(); i++) {
    next = dynamic_cast<Track*>(track_input_->GetConnectedNode(i));

    if (next) {
      break;
    }
  }

  int track_index;

  if (next) {
    // Insert track before "next"
    track_index = track_cache_.indexOf(next);
    track_cache_.insert(track_index, track);
  } else {
    // No "next", this track must come at the end
    track_index = track_cache_.size();
    track_cache_.append(track);
  }

  // Update track indexes in the list (including this track)
  UpdateTrackIndexesFrom(track_index);

  connect(track, &Track::TrackLengthChanged, this, &TrackList::UpdateTotalLength);

  track->set_type(type_);

  emit TrackListChanged();

  // This function must be called after the track is added to track_cache_, since it uses track_cache_ to determine
  // the track's index
  emit TrackAdded(track);

  UpdateTotalLength();
}

void TrackList::TrackDisconnected(Node *node, int element)
{
  Q_UNUSED(element)

  Track* track = dynamic_cast<Track*>(node);

  if (!track) {
    return;
  }

  int index_of_track = track_cache_.indexOf(track);
  track_cache_.removeAt(index_of_track);

  // Update indices for all subsequent tracks
  UpdateTrackIndexesFrom(index_of_track);

  // Traverse through Tracks uncaching and disconnecting them
  emit TrackRemoved(track);

  track->SetIndex(-1);
  track->set_type(Track::kNone);

  disconnect(track, &Track::TrackLengthChanged, this, &TrackList::UpdateTotalLength);

  emit TrackListChanged();

  UpdateTotalLength();
}

void TrackList::UpdateTrackIndexesFrom(int index)
{
  for (int i=index; i<track_cache_.size(); i++) {
    track_cache_.at(i)->SetIndex(i);
  }
}

NodeGraph *TrackList::GetParentGraph() const
{
  return static_cast<NodeGraph*>(parent()->parent());
}

void TrackList::UpdateTotalLength()
{
  total_length_ = 0;

  foreach (Track* track, track_cache_) {
    if (track) {
      total_length_ = qMax(total_length_, track->track_length());
    }
  }

  emit LengthChanged(total_length_);
}

}

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "node/blend/alphaover/alphaover.h"
#include "timeline.h"

TrackList::TrackList(TimelineOutput* parent, NodeInput *track_input) :
  QObject(parent),
  track_input_(track_input)
{
  connect(parent, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackConnectionAdded(NodeEdgePtr)));
  connect(parent, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackConnectionRemoved(NodeEdgePtr)));
}

TrackOutput *TrackList::attached_track()
{
  return Node::ValueToPtr<TrackOutput>(track_input_->get_value(0));
}

void TrackList::AttachTrack(TrackOutput *track)
{
  TrackOutput* current_track = track;

  // Traverse through Tracks caching and connecting them
  while (current_track != nullptr) {
    connect(current_track, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackEdgeAdded(NodeEdgePtr)));
    connect(current_track, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackEdgeRemoved(NodeEdgePtr)));
    connect(current_track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    connect(current_track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));
    connect(current_track, SIGNAL(Refreshed()), this, SLOT(UpdateTotalLength()));

    current_track->SetIndex(track_cache_.size());

    track_cache_.append(current_track);
    emit TrackListChanged();

    // This function must be called after the track is added to track_cache_, since it uses track_cache_ to determine
    // the track's index
    emit TrackAdded(current_track);

    current_track = current_track->next_track();
  }

  UpdateTotalLength();
}

void TrackList::DetachTrack(TrackOutput *track)
{
  TrackOutput* current_track = track;

  // Traverse through Tracks uncaching and disconnecting them
  while (current_track != nullptr) {
    emit TrackRemoved(current_track);

    current_track->SetIndex(-1);

    disconnect(current_track, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackEdgeAdded(NodeEdgePtr)));
    disconnect(current_track, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackEdgeRemoved(NodeEdgePtr)));
    disconnect(current_track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    disconnect(current_track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));
    disconnect(current_track, SIGNAL(Refreshed()), this, SLOT(UpdateTotalLength()));

    track_cache_.removeAll(current_track);
    emit TrackListChanged();

    current_track = current_track->next_track();
  }

  UpdateTotalLength();
}

void TrackList::TrackAddedBlock(Block *block)
{
  emit BlockAdded(block, static_cast<TrackOutput*>(sender())->Index());
}

void TrackList::TrackRemovedBlock(Block *block)
{
  emit BlockRemoved(block);
}

const QVector<TrackOutput *> &TrackList::Tracks()
{
  return track_cache_;
}

TrackOutput *TrackList::TrackAt(int index)
{
  return track_cache_.at(index);
}

const rational &TrackList::Timebase()
{
  return timebase_;
}

void TrackList::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  emit TimebaseChanged(timebase_);
}

const rational &TrackList::TrackLength()
{
  return total_length_;
}

void TrackList::AddTrack()
{
  TrackOutput* track = new TrackOutput();
  GetParentGraph()->AddNode(track);

  if (track_cache_.isEmpty()) {
    // Connect this track directly to this output
    NodeParam::ConnectEdge(track->track_output(), track_input_);
  } else {
    TrackOutput* current_last_track = track_cache_.last();

    // Connect this track to the current last track
    NodeParam::ConnectEdge(track->track_output(), current_last_track->track_input());

    // FIXME: Test code only
    if (current_last_track->texture_output()->IsConnected()) {
      AlphaOverBlend* blend = new AlphaOverBlend();
      GetParentGraph()->AddNode(blend);

      NodeParam::ConnectEdge(track->texture_output(), blend->blend_input());
      NodeParam::ConnectEdge(current_last_track->texture_output(), blend->base_input());
      NodeParam::ConnectEdge(blend->texture_output(), current_last_track->texture_output()->edges().first()->input());
    }
    // End test code
  }
}

void TrackList::RemoveTrack()
{
  if (track_cache_.isEmpty()) {
    return;
  }

  TrackOutput* track = track_cache_.last();

  GetParentGraph()->TakeNode(track);

  delete track;
}

void TrackList::TrackConnectionAdded(NodeEdgePtr edge)
{
  if (edge->input() != track_input_) {
    return;
  }

  AttachTrack(attached_track());

  // FIXME: Is this necessary?
  emit TimebaseChanged(timebase_);
}

void TrackList::TrackConnectionRemoved(NodeEdgePtr edge)
{
  if (edge->input() != track_input_) {
    return;
  }

  DetachTrack(Node::ValueToPtr<TrackOutput>(edge->output()->get_value(0)));

  emit TimelineCleared();
}

void TrackList::TrackEdgeAdded(NodeEdgePtr edge)
{
  // Assume this signal was sent from a TrackOutput
  TrackOutput* track = static_cast<TrackOutput*>(sender());

  // If this edge pertains to the track's track input, all the tracks just added need attaching
  if (edge->input() == track->track_input()) {
    TrackOutput* added_track = Node::ValueToPtr<TrackOutput>(edge->output()->get_value(0));

    AttachTrack(added_track);
  }
}

void TrackList::TrackEdgeRemoved(NodeEdgePtr edge)
{
  // Assume this signal was sent from a TrackOutput
  TrackOutput* track = static_cast<TrackOutput*>(sender());

  // If this edge pertains to the track's track input, all the tracks just added need attaching
  if (edge->input() == track->track_input()) {
    TrackOutput* added_track = Node::ValueToPtr<TrackOutput>(edge->output()->get_value(0));

    DetachTrack(added_track);
  }
}

NodeGraph *TrackList::GetParentGraph()
{
  return static_cast<NodeGraph*>(parent()->parent());
}

void TrackList::UpdateTotalLength()
{
  total_length_ = 0;

  foreach (TrackOutput* track, track_cache_) {
    total_length_ = qMax(total_length_, track->in());
  }

  emit LengthChanged(total_length_);
}

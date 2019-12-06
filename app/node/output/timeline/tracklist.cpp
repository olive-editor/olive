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

TrackList::TrackList(TimelineOutput* parent, const enum TrackType &type, NodeInputArray *track_input) :
  QObject(parent),
  track_input_(track_input),
  type_(type)
{
  connect(track_input, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(TrackConnected(NodeEdgePtr)));
  connect(track_input, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(TrackDisconnected(NodeEdgePtr)));
  connect(track_input, SIGNAL(SizeChanged(int)), this, SLOT(TrackListSizeChanged(int)));
}

void TrackList::TrackAddedBlock(Block *block)
{
  emit BlockAdded(block, static_cast<TrackOutput*>(sender())->Index());
}

void TrackList::TrackRemovedBlock(Block *block)
{
  emit BlockRemoved(block);
}

void TrackList::TrackListSizeChanged(int size)
{
  int old_size = track_cache_.size();

  track_cache_.resize(size);

  // Fill new slots with nullptr
  for (int i=old_size;i<size;i++) {
    track_cache_.replace(i, nullptr);
  }
}

const QVector<TrackOutput *> &TrackList::Tracks()
{
  return track_cache_;
}

TrackOutput *TrackList::TrackAt(int index)
{
  if (index < 0 || index >= track_cache_.size()) {
    return nullptr;
  }

  return track_cache_.at(index);
}

const rational &TrackList::TrackLength()
{
  return total_length_;
}

const enum TrackType &TrackList::TrackType()
{
  return type_;
}

TrackOutput* TrackList::AddTrack()
{
  TrackOutput* track = new TrackOutput();
  GetParentGraph()->AddNode(track);

  track_input_->Append();

  NodeInput* assoc_input = track_input_->At(track_input_->GetSize() - 1);

  // Connect this track directly to this output
  NodeParam::ConnectEdge(track->output(), assoc_input);

  // FIXME: Test code only
  /*if (track_input_->GetSize() > 1) {
    if (current_last_track->output()->IsConnected()) {
      AlphaOverBlend* blend = new AlphaOverBlend();
      GetParentGraph()->AddNode(blend);

      NodeParam::ConnectEdge(track->output(), blend->blend_input());
      NodeParam::ConnectEdge(current_last_track->output(), blend->base_input());
      NodeParam::ConnectEdge(blend->output(), current_last_track->output()->edges().first()->input());
    }
  }*/
  // End test code

  // Connect this track to the current last track
  NodeParam::ConnectEdge(track->output(), assoc_input);

  return track;
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

void TrackList::TrackConnected(NodeEdgePtr edge)
{
  int track_index = track_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(track_index >= 0);

  Node* connected_node = edge->output()->parentNode();

  if (connected_node->IsTrack()) {
    TrackOutput* connected_track = static_cast<TrackOutput*>(connected_node);// Traverse through Tracks caching and connecting them

    track_cache_.replace(track_index, connected_track);

    connect(connected_track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    connect(connected_track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));
    connect(connected_track, SIGNAL(TrackLengthChanged()), this, SLOT(UpdateTotalLength()));

    connected_track->SetIndex(track_index);
    connected_track->set_track_type(type_);

    emit TrackListChanged();

    // This function must be called after the track is added to track_cache_, since it uses track_cache_ to determine
    // the track's index
    emit TrackAdded(connected_track);

    UpdateTotalLength();

  }
}

void TrackList::TrackDisconnected(NodeEdgePtr edge)
{
  int track_index = track_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(track_index >= 0);

  TrackOutput* track = track_cache_.at(track_index);

  if (track) {
    track_cache_.replace(track_index, nullptr);

    // Traverse through Tracks uncaching and disconnecting them
    emit TrackRemoved(track);

    track->SetIndex(-1);
    track->set_track_type(kTrackTypeNone);

    disconnect(track, SIGNAL(BlockAdded(Block*)), this, SLOT(TrackAddedBlock(Block*)));
    disconnect(track, SIGNAL(BlockRemoved(Block*)), this, SLOT(TrackRemovedBlock(Block*)));
    disconnect(track, SIGNAL(TrackLengthChanged()), this, SLOT(UpdateTotalLength()));

    emit TrackListChanged();

    UpdateTotalLength();
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
    if (track) {
      total_length_ = qMax(total_length_, track->track_length());
    }
  }

  emit LengthChanged(total_length_);
}

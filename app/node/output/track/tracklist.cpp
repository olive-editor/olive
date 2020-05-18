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

#include "node/factory.h"
#include "node/math/math/math.h"
#include "node/math/merge/merge.h"
#include "node/output/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER

TrackList::TrackList(ViewerOutput *parent, const Timeline::TrackType &type, NodeInputArray *track_input) :
  QObject(parent),
  track_input_(track_input),
  type_(type)
{
  connect(track_input_, &NodeInputArray::SubParamEdgeAdded, this, &TrackList::TrackConnected);
  connect(track_input_, &NodeInputArray::SubParamEdgeRemoved, this, &TrackList::TrackDisconnected);
}

const Timeline::TrackType &TrackList::type() const
{
  return type_;
}

void TrackList::TrackAddedBlock(Block *block)
{
  emit BlockAdded(block, static_cast<TrackOutput*>(sender())->Index());
}

void TrackList::TrackRemovedBlock(Block *block)
{
  emit BlockRemoved(block);
}

const QVector<TrackOutput *> &TrackList::GetTracks() const
{
  return track_cache_;
}

TrackOutput *TrackList::GetTrackAt(int index) const
{
  if (index < track_cache_.size()) {
    return track_cache_.at(index);
  } else {
    return nullptr;
  }
}

const rational &TrackList::GetTotalLength() const
{
  return total_length_;
}

int TrackList::GetTrackCount() const
{
  return track_cache_.size();
}

TrackOutput* TrackList::AddTrack()
{
  TrackOutput* track = new TrackOutput();
  GetParentGraph()->AddNode(track);

  track_input_->Append();

  // Connect this track directly to this output
  NodeParam::ConnectEdge(track->output(),
                         track_input_->At(track_input_->GetSize() - 1));

  // Auto-merge with previous track
  if (track_input_->GetSize() > 1) {
    TrackOutput* last_track = nullptr;

    for (int i=track_cache_.size()-1;i>=0;i--) {
      TrackOutput* test_track = track_cache_.at(i);

      if (test_track && test_track != track) {
        last_track = test_track;
        break;
      }
    }

    if (last_track && last_track->output()->IsConnected()) {
      foreach (NodeEdgePtr edge, last_track->output()->edges()) {
        if (!track_input_->ContainsSubParameter(edge->input())) {
          switch (type_) {
          case Timeline::kTrackTypeVideo:
          {
            MergeNode* blend = new MergeNode();
            GetParentGraph()->AddNode(blend);

            NodeParam::ConnectEdge(track->output(), blend->blend_in());
            NodeParam::ConnectEdge(last_track->output(), blend->base_in());
            NodeParam::ConnectEdge(blend->output(), edge->input());
            break;
          }
          case Timeline::kTrackTypeAudio:
          {
            MathNode* add = new MathNode();
            GetParentGraph()->AddNode(add);

            NodeParam::ConnectEdge(track->output(), add->param_a_in());
            NodeParam::ConnectEdge(last_track->output(), add->param_b_in());
            NodeParam::ConnectEdge(add->output(), edge->input());
            break;
          }
          default:
            break;
          }
        }
      }
    }
  }

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

  track_input_->RemoveLast();
}

void TrackList::TrackConnected(NodeEdgePtr edge)
{
  int track_index = track_input_->IndexOfSubParameter(edge->input());

  Q_ASSERT(track_index >= 0);

  Node* connected_node = edge->output()->parentNode();

  if (connected_node->IsTrack()) {
    TrackOutput* connected_track = static_cast<TrackOutput*>(connected_node);

    {
      // Find "real" index
      TrackOutput* next = nullptr;
      for (int i=track_index+1; i<track_input_->GetSize(); i++) {
        Node* that_track = track_input_->At(i)->get_connected_node();

        if (that_track && that_track->IsTrack()) {
          next = static_cast<TrackOutput*>(that_track);
          break;
        }
      }

      int track_index;

      if (next) {
        // Insert track before "next"
        track_index = track_cache_.indexOf(next);
        track_cache_.insert(track_index, connected_track);
      } else {
        // No "next", this track must come at the end
        track_index = track_cache_.size();
        track_cache_.append(connected_track);
      }

      connected_track->SetIndex(track_index);
    }

    connect(connected_track, &TrackOutput::BlockAdded, this, &TrackList::TrackAddedBlock);
    connect(connected_track, &TrackOutput::BlockRemoved, this, &TrackList::TrackRemovedBlock);
    connect(connected_track, &TrackOutput::TrackLengthChanged, this, &TrackList::UpdateTotalLength);
    connect(connected_track, &TrackOutput::TrackHeightChanged, this, &TrackList::TrackHeightChangedSlot);

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

  Node* connected_node = edge->output()->parentNode();

  if (connected_node->IsTrack()) {
    TrackOutput* track = static_cast<TrackOutput*>(connected_node);

    int index_of_track = track_cache_.indexOf(track);
    track_cache_.removeAt(index_of_track);

    // Update indices for all subsequent tracks
    for (int i=index_of_track; i<track_cache_.size(); i++) {
      track_cache_.at(i)->SetIndex(i);
    }

    // Traverse through Tracks uncaching and disconnecting them
    emit TrackRemoved(track);

    track->SetIndex(-1);
    track->set_track_type(Timeline::kTrackTypeNone);

    disconnect(track, &TrackOutput::BlockAdded, this, &TrackList::TrackAddedBlock);
    disconnect(track, &TrackOutput::BlockRemoved, this, &TrackList::TrackRemovedBlock);
    disconnect(track, &TrackOutput::TrackLengthChanged, this, &TrackList::UpdateTotalLength);
    disconnect(track, &TrackOutput::TrackHeightChanged, this, &TrackList::TrackHeightChangedSlot);

    emit TrackListChanged();

    UpdateTotalLength();
  }
}

NodeGraph *TrackList::GetParentGraph() const
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

void TrackList::TrackHeightChangedSlot(int height)
{
  emit TrackHeightChanged(static_cast<TrackOutput*>(sender())->Index(), height);
}

OLIVE_NAMESPACE_EXIT

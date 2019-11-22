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

#include "timeline.h"

#include <QDebug>

#include "node/block/gap/gap.h"
#include "node/graph.h"
#include "panel/timeline/timeline.h"

TimelineOutput::TimelineOutput()
{
  // Create TrackList instances
  track_inputs_.resize(kTrackTypeCount);
  track_lists_.resize(kTrackTypeCount);

  for (int i=0;i<kTrackTypeCount;i++) {
    // Create track input
    NodeInput* track_input = new NodeInput(QString("track_in_%1").arg(i));
    track_input->set_data_type(NodeParam::kTrack);
    AddParameter(track_input);
    track_inputs_.replace(i, track_input);

    TrackList* list = new TrackList(this, static_cast<TrackType>(i), track_input);
    track_lists_.replace(i, list);
    connect(list, SIGNAL(TrackListChanged()), this, SLOT(UpdateTrackCache()));
    connect(list, SIGNAL(LengthChanged(const rational &)), this, SLOT(UpdateLength(const rational &)));
    connect(list, SIGNAL(BlockAdded(Block*, int)), this, SLOT(TrackListAddedBlock(Block*, int)));
    connect(list, SIGNAL(BlockRemoved(Block*)), this, SIGNAL(BlockRemoved(Block*)));
    connect(list, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(TrackListAddedTrack(TrackOutput*)));
    connect(list, SIGNAL(TrackRemoved(TrackOutput*)), this, SIGNAL(TrackRemoved(TrackOutput*)));
  }

  length_output_ = new NodeOutput("length_out");
  AddParameter(length_output_);
}

Node *TimelineOutput::copy()
{
  return new TimelineOutput();
}

QString TimelineOutput::Name()
{
  return tr("Timeline");
}

QString TimelineOutput::id()
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category()
{
  return tr("Output");
}

QString TimelineOutput::Description()
{
  return tr("Node for communicating between a Timeline panel and the node graph.");
}

QVector<TrackOutput *> TimelineOutput::Tracks()
{
  return track_cache_;
}

NodeOutput *TimelineOutput::length_output()
{
  return length_output_;
}

const rational &TimelineOutput::timeline_length()
{
  return length_;
}

const rational &TimelineOutput::Timebase()
{
  return timebase_;
}

QVariant TimelineOutput::Value(NodeOutput *output)
{
  if (output == length_output_) {
    return QVariant::fromValue(length_);
  }

  return 0;
}

void TimelineOutput::UpdateTrackCache()
{
  track_cache_.clear();

  foreach (TrackList* list, track_lists_) {
    track_cache_.append(list->Tracks());
  }
}

void TimelineOutput::UpdateLength(const rational &length)
{
  // If this length is equal, no-op
  if (length == length_) {
    return;
  }

  // If this length is greater, this must be the new total length
  if (length > length_) {
    length_ = length;
    emit LengthChanged(length_);
    return;
  }

  // Otherwise, the new length is shorter and we'll have to manually determine what the new max length is
  rational new_length = 0;

  foreach (TrackList* list, track_lists_) {
    new_length = qMax(new_length, list->TrackLength());
  }

  if (new_length != length_) {
    length_ = new_length;
    emit LengthChanged(length_);
  }
}

void TimelineOutput::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  emit TimebaseChanged(timebase_);
}

NodeInput *TimelineOutput::track_input(TrackType type)
{
  return track_inputs_.at(type);
}

TrackList *TimelineOutput::track_list(TrackType type)
{
  return track_lists_.at(type);
}

void TimelineOutput::TrackListAddedBlock(Block *block, int index)
{
  TrackType type = static_cast<TrackList*>(sender())->TrackType();
  emit BlockAdded(block, TrackReference(type, index));
}

void TimelineOutput::TrackListAddedTrack(TrackOutput *track)
{
  TrackType type = static_cast<TrackList*>(sender())->TrackType();
  emit TrackAdded(track, type);
}

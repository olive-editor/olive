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
    NodeInputArray* track_input = new NodeInputArray(QStringLiteral("track_in_%1").arg(i), NodeParam::kAny);
    AddInput(track_input);
    track_inputs_.replace(i, track_input);

    TrackList* list = new TrackList(this, static_cast<TrackType>(i), track_input);
    track_lists_.replace(i, list);
    connect(list, SIGNAL(TrackListChanged()), this, SLOT(UpdateTrackCache()));
    connect(list, SIGNAL(LengthChanged(const rational &)), this, SLOT(UpdateLength(const rational &)));
    connect(list, SIGNAL(BlockAdded(Block*, int)), this, SLOT(TrackListAddedBlock(Block*, int)));
    connect(list, SIGNAL(BlockRemoved(Block*)), this, SIGNAL(BlockRemoved(Block*)));
    connect(list, SIGNAL(TrackAdded(TrackOutput*)), this, SLOT(TrackListAddedTrack(TrackOutput*)));
    connect(list, SIGNAL(TrackRemoved(TrackOutput*)), this, SIGNAL(TrackRemoved(TrackOutput*)));
    connect(list, SIGNAL(TrackHeightChanged(int, int)), this, SLOT(TrackHeightChangedSlot(int, int)));
  }
}

Node *TimelineOutput::copy() const
{
  return new TimelineOutput();
}

QString TimelineOutput::Name() const
{
  return tr("Timeline");
}

QString TimelineOutput::id() const
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category() const
{
  return tr("Output");
}

QString TimelineOutput::Description() const
{
  return tr("Node for communicating between a Timeline panel and the node graph.");
}

const rational &TimelineOutput::length() const
{
  return length_;
}

const QVector<TrackOutput *>& TimelineOutput::Tracks() const
{
  return track_cache_;
}

const rational &TimelineOutput::timeline_length() const
{
  return length_;
}

const rational &TimelineOutput::timebase() const
{
  return timebase_;
}

NodeValueTable TimelineOutput::Value(const NodeValueDatabase &value) const
{
  NodeValueTable table = value.Merge();
  table.Push(NodeParam::kRational, QVariant::fromValue(length()), "length");
  return table;
}

void TimelineOutput::UpdateTrackCache()
{
  track_cache_.clear();

  foreach (TrackList* list, track_lists_) {
    QVector<TrackOutput*> track_list = list->Tracks();

    foreach (TrackOutput* track, track_list) {
      if (track) {
        track_cache_.append(list->Tracks());
      }
    }
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

void TimelineOutput::Retranslate()
{
  for (int i=0;i<track_inputs_.size();i++) {
    QString input_name;

    switch (static_cast<TrackType>(i)) {
    case kTrackTypeVideo:
      input_name = tr("Video Tracks");
      break;
    case kTrackTypeAudio:
      input_name = tr("Audio Tracks");
      break;
    case kTrackTypeSubtitle:
      input_name = tr("Subtitle Tracks");
      break;
    case kTrackTypeNone:
    case kTrackTypeCount:
      break;
    }

    if (!input_name.isEmpty())
      track_inputs_.at(i)->set_name(input_name);
  }
}

NodeInput *TimelineOutput::track_input(TrackType type) const
{
  return track_inputs_.at(type);
}

TrackList *TimelineOutput::track_list(TrackType type) const
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

void TimelineOutput::TrackHeightChangedSlot(int index, int height)
{
  emit TrackHeightChanged(static_cast<TrackList*>(sender())->type(), index, height);
}

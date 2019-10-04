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
    track_input->add_data_input(NodeParam::kTrack);
    AddParameter(track_input);
    track_inputs_[i] = track_input;

    TrackList* list = new TrackList(this, track_input);
    track_lists_[i] = list;
    connect(list, SIGNAL(TrackListChanged()), this, SLOT(UpdateTrackCache()));
  }

  length_output_ = new NodeOutput("length_out");
  length_output_->set_data_type(NodeParam::kRational);
  AddParameter(length_output_);
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

QVariant TimelineOutput::Value(NodeOutput *output, const rational &time)
{
  if (output == length_output_) {
    Q_UNUSED(time)

    return QVariant::fromValue(timeline_length());
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

rational TimelineOutput::timeline_length()
{
  rational length = 0;

  foreach (TrackList* list, track_lists_) {
    length = qMax(list->TrackListLength(), length);
  }

  return length;
}

void TimelineOutput::SetTimebase(const rational &timebase)
{
  foreach (TrackList* list, track_lists_) {
    list->SetTimebase(timebase);
  }
}

NodeInput *TimelineOutput::track_input(TimelineOutput::TrackType type)
{
  return track_inputs_.at(type);
}

TrackList *TimelineOutput::track_list(TimelineOutput::TrackType type)
{
  return track_lists_.at(type);
}

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

#include "sequence.h"

#include <QCoreApplication>

#include "common/channellayout.h"
#include "common/timecodefunctions.h"
#include "panel/panelmanager.h"
#include "panel/node/node.h"
#include "panel/timeline/timeline.h"
#include "panel/viewer/viewer.h"
#include "ui/icons/icons.h"

Sequence::Sequence() :
  timeline_output_(nullptr),
  viewer_output_(nullptr),
  video_track_output_(nullptr),
  audio_track_output_(nullptr)
{
}

void Sequence::Open(SequencePtr sequence)
{
  // FIXME: This is fairly "hardcoded" behavior and doesn't support infinite panels

  ViewerPanel* viewer_panel = olive::panel_manager->MostRecentlyFocused<ViewerPanel>();
  TimelinePanel* timeline_panel = olive::panel_manager->MostRecentlyFocused<TimelinePanel>();
  NodePanel* node_panel = olive::panel_manager->MostRecentlyFocused<NodePanel>();

  viewer_panel->ConnectViewerNode(sequence->viewer_output_);
  timeline_panel->ConnectTimelineNode(sequence->timeline_output_);
  node_panel->SetGraph(sequence.get());

  connect(timeline_panel, SIGNAL(TimeChanged(const int64_t&)), viewer_panel, SLOT(SetTime(const int64_t&)));
  connect(viewer_panel, SIGNAL(TimeChanged(const int64_t&)), timeline_panel, SLOT(SetTime(const int64_t&)));
}

void Sequence::add_default_nodes()
{
  timeline_output_ = new TimelineOutput();
  timeline_output_->SetCanBeDeleted(false);
  AddNode(timeline_output_);

  viewer_output_ = new ViewerOutput();
  viewer_output_->SetCanBeDeleted(false);
  AddNode(viewer_output_);

  video_track_output_ = new TrackOutput();
  video_track_output_->SetCanBeDeleted(false);
  AddNode(video_track_output_);

  audio_track_output_ = new TrackOutput();
  audio_track_output_->SetCanBeDeleted(false);
  AddNode(audio_track_output_);

  // Connect tracks to viewer
  NodeParam::ConnectEdge(video_track_output_->buffer_output(), viewer_output_->texture_input());
  NodeParam::ConnectEdge(audio_track_output_->buffer_output(), viewer_output_->samples_input());

  // Connect timeline length to viewer
  NodeParam::ConnectEdge(timeline_output_->length_output(), viewer_output_->length_input());

  // Connect track to timeline
  NodeParam::ConnectEdge(video_track_output_->track_output(), timeline_output_->track_input(kTrackTypeVideo));
  NodeParam::ConnectEdge(audio_track_output_->track_output(), timeline_output_->track_input(kTrackTypeAudio));

  // Update the timebase on these nodes
  set_video_params(video_params_);
}

Item::Type Sequence::type() const
{
  return kSequence;
}

QIcon Sequence::icon()
{
  return olive::icon::Sequence;
}

QString Sequence::duration()
{
  if (timeline_output_ == nullptr) {
    return QString();
  }

  rational timeline_length = timeline_output_->length_output()->get_value(0, 0).value<rational>();

  int64_t timestamp = olive::time_to_timestamp(timeline_length, video_params_.time_base());

  return olive::timestamp_to_timecode(timestamp, video_params_.time_base(), olive::CurrentTimecodeDisplay());
}

QString Sequence::rate()
{
  return QCoreApplication::translate("Sequence", "%1 FPS").arg(video_params_.time_base().flipped().toDouble());
}

const VideoParams &Sequence::video_params()
{
  return video_params_;
}

void Sequence::set_video_params(const VideoParams &vparam)
{
  video_params_ = vparam;

  if (viewer_output_ != nullptr)
    viewer_output_->set_video_params(video_params_);

  if (timeline_output_ != nullptr)
    timeline_output_->SetTimebase(video_params_.time_base());
}

const AudioParams &Sequence::audio_params()
{
  return audio_params_;
}

void Sequence::set_audio_params(const AudioParams &params)
{
  audio_params_ = params;

  if (viewer_output_ != nullptr)
    viewer_output_->set_audio_params(audio_params_);
}

void Sequence::set_default_parameters()
{
  // FIXME: Make these configurable (hardcoded)
  set_video_params(VideoParams(1920, 1080, rational(1001, 30000)));
  set_audio_params(AudioParams(48000, AV_CH_LAYOUT_STEREO));
}

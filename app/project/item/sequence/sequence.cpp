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

#include "config/config.h"
#include "common/channellayout.h"
#include "common/timecodefunctions.h"
#include "panel/panelmanager.h"
#include "panel/node/node.h"
#include "panel/curve/curve.h"
#include "panel/param/param.h"
#include "panel/timeline/timeline.h"
#include "panel/viewer/viewer.h"
#include "ui/icons/icons.h"

Sequence::Sequence() :
  viewer_output_(nullptr)
{
}

void Sequence::Open(Sequence* sequence)
{
  // FIXME: This is fairly "hardcoded" behavior and doesn't support infinite panels

  ViewerPanel* viewer_panel = PanelManager::instance()->MostRecentlyFocused<ViewerPanel>();
  TimelinePanel* timeline_panel = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();
  NodePanel* node_panel = PanelManager::instance()->MostRecentlyFocused<NodePanel>();

  viewer_panel->ConnectViewerNode(sequence->viewer_output_);
  timeline_panel->ConnectTimelineNode(sequence->viewer_output_);
  node_panel->SetGraph(sequence);
}

void Sequence::add_default_nodes()
{
  viewer_output_ = new ViewerOutput();
  viewer_output_->SetCanBeDeleted(false);
  AddNode(viewer_output_);

  // Create tracks and connect them to the viewer
  Node* video_track_output = viewer_output_->track_list(Timeline::kTrackTypeVideo)->AddTrack();
  Node* audio_track_output = viewer_output_->track_list(Timeline::kTrackTypeAudio)->AddTrack();
  NodeParam::ConnectEdge(video_track_output->output(), viewer_output_->texture_input());
  NodeParam::ConnectEdge(audio_track_output->output(), viewer_output_->samples_input());
  //timeline_output_->track_list(TrackType::kTrackTypeVideo)->AddTrack();

  // Update the timebase on these nodes
  set_video_params(video_params_);
  set_audio_params(audio_params_);
}

Item::Type Sequence::type() const
{
  return kSequence;
}

QIcon Sequence::icon()
{
  return icon::Sequence;
}

QString Sequence::duration()
{
  if (!viewer_output_) {
    return QString();
  }

  rational timeline_length = viewer_output_->Length();

  int64_t timestamp = Timecode::time_to_timestamp(timeline_length, video_params_.time_base());

  return Timecode::timestamp_to_timecode(timestamp, video_params_.time_base(), Timecode::CurrentDisplay());
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
  set_video_params(VideoParams(Config::Current()["DefaultSequenceWidth"].toInt(),
                               Config::Current()["DefaultSequenceHeight"].toInt(),
                               Config::Current()["DefaultSequenceFrameRate"].value<rational>()));
  set_audio_params(AudioParams(Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                               Config::Current()["DefaultSequenceAudioLayout"].toULongLong()));
}

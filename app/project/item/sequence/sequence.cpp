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
  renderer_processor_(nullptr),
  viewer_output_(nullptr),
  track_output_(nullptr)
{
}

void Sequence::Open(SequencePtr sequence)
{
  olive::panel_manager->MostRecentlyFocused<ViewerPanel>()->ConnectViewerNode(sequence->viewer_output_);
  olive::panel_manager->MostRecentlyFocused<TimelinePanel>()->ConnectTimelineNode(sequence->timeline_output_);
  olive::panel_manager->MostRecentlyFocused<NodePanel>()->SetGraph(sequence.get());
}

void Sequence::AddDefaultNodes()
{
  timeline_output_ = new TimelineOutput();
  timeline_output_->SetCanBeDeleted(false);
  AddNode(timeline_output_);

  renderer_processor_ = new RendererProcessor();
  renderer_processor_->SetCanBeDeleted(false);
  AddNode(renderer_processor_);

  viewer_output_ = new ViewerOutput();
  viewer_output_->SetCanBeDeleted(false);
  AddNode(viewer_output_);

  track_output_ = new TrackOutput();
  track_output_->SetCanBeDeleted(false);
  AddNode(track_output_);

  // Connect track to renderer
  NodeParam::ConnectEdge(track_output_->texture_output(), renderer_processor_->texture_input());

  // Connect renderer to viewer
  NodeParam::ConnectEdge(renderer_processor_->texture_output(), viewer_output_->texture_input());

  // Connect track to timeline
  NodeParam::ConnectEdge(track_output_->track_output(), timeline_output_->track_input());

  // Connect timeline end point to renderer
  NodeParam::ConnectEdge(timeline_output_->length_output(), renderer_processor_->length_input());

  // Update the timebase on these nodes
  set_video_time_base(video_time_base_);
  update_parameters();
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

  rational timeline_length = timeline_output_->length_output()->get_value(0).value<rational>();

  int64_t timestamp = olive::time_to_timestamp(timeline_length, video_time_base_);

  return olive::timestamp_to_timecode(timestamp, video_time_base_, olive::CurrentTimecodeDisplay());
}

QString Sequence::rate()
{
  return QCoreApplication::translate("Sequence", "%1 FPS").arg(video_time_base_.flipped().toDouble());
}

const int &Sequence::video_width()
{
  return video_width_;
}

void Sequence::set_video_width(const int &width)
{
  video_width_ = width;

  update_parameters();
}

const int &Sequence::video_height() const
{
  return video_height_;
}

void Sequence::set_video_height(const int &height)
{
  video_height_ = height;

  update_parameters();
}

const rational &Sequence::video_time_base()
{
  return video_time_base_;
}

void Sequence::set_video_time_base(const rational &time_base)
{
  video_time_base_ = time_base;

  if (timeline_output_ != nullptr)
    timeline_output_->SetTimebase(video_time_base_);

  if (viewer_output_ != nullptr)
    viewer_output_->SetTimebase(video_time_base_);

  if (renderer_processor_ != nullptr)
    renderer_processor_->SetTimebase(video_time_base_);
}

const rational &Sequence::audio_time_base()
{
  return audio_time_base_;
}

void Sequence::set_audio_time_base(const rational &time_base)
{
  audio_time_base_ = time_base;
}

const uint64_t &Sequence::audio_channel_layout()
{
  return audio_channel_layout_;
}

void Sequence::set_audio_channel_layout(const uint64_t &channel_layout)
{
  audio_channel_layout_ = channel_layout;
}

void Sequence::SetDefaultParameters()
{
  // FIXME: Make these configurable
  set_video_width(1920);
  set_video_height(1080);
  set_video_time_base(rational(1001, 30000));

  set_audio_time_base(rational(1, 48000));
  set_audio_channel_layout(AV_CH_LAYOUT_STEREO);
}

void Sequence::update_parameters()
{
  if (renderer_processor_ != nullptr) {
    // Set renderer's parameters based on sequence's parameters
    renderer_processor_->SetParameters(video_width_,
                                       video_height_,
                                       olive::PIX_FMT_RGBA16F, // FIXME: Make this configurable
                                       olive::RenderMode::kOffline,
                                       2);

    // Set the "cache name" only here to aid the cache ID's uniqueness
    renderer_processor_->SetCacheName(name());
  }
}

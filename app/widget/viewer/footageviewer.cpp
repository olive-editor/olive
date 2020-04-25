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

#include "footageviewer.h"

#include <QDrag>
#include <QMimeData>

#include "project/project.h"

OLIVE_NAMESPACE_ENTER

FootageViewerWidget::FootageViewerWidget(QWidget *parent) :
  ViewerWidget(parent),
  footage_(nullptr)
{
  video_node_ = new VideoInput();
  audio_node_ = new AudioInput();
  viewer_node_ = new ViewerOutput();

  connect(main_gl_widget(), &ViewerDisplayWidget::DragStarted, this, &FootageViewerWidget::StartFootageDrag);

  controls_->SetAudioVideoDragButtonsVisible(true);
  connect(controls_, &PlaybackControls::VideoPressed, this, &FootageViewerWidget::StartVideoDrag);
  connect(controls_, &PlaybackControls::AudioPressed, this, &FootageViewerWidget::StartAudioDrag);
}

Footage *FootageViewerWidget::GetFootage() const
{
  return footage_;
}

void FootageViewerWidget::SetFootage(Footage *footage)
{
  if (footage_) {
    ConnectViewerNode(nullptr);

    NodeParam::DisconnectEdge(video_node_->output(), viewer_node_->texture_input());
    NodeParam::DisconnectEdge(audio_node_->output(), viewer_node_->samples_input());
  }

  footage_ = footage;

  if (footage_) {
    VideoStreamPtr video_stream = nullptr;
    AudioStreamPtr audio_stream = nullptr;

    foreach (StreamPtr s, footage->streams()) {
      if (!audio_stream && s->type() == Stream::kAudio) {
        audio_stream = std::static_pointer_cast<AudioStream>(s);
      }

      if (!video_stream
          && (s->type() == Stream::kVideo || s->type() == Stream::kImage)) {
        video_stream = std::static_pointer_cast<VideoStream>(s);
      }

      if (audio_stream && video_stream) {
        break;
      }
    }

    if (video_stream) {
      video_node_->SetFootage(video_stream);
      viewer_node_->set_video_params(VideoParams(video_stream->width(), video_stream->height(), video_stream->frame_rate().flipped()));
      NodeParam::ConnectEdge(video_node_->output(), viewer_node_->texture_input());
    }

    if (audio_stream) {
      audio_node_->SetFootage(audio_stream);
      viewer_node_->set_audio_params(AudioParams(audio_stream->sample_rate(), audio_stream->channel_layout()));
      NodeParam::ConnectEdge(audio_node_->output(), viewer_node_->samples_input());
    }

    ConnectViewerNode(viewer_node_, footage->project()->color_manager());
  }
}

TimelinePoints *FootageViewerWidget::ConnectTimelinePoints()
{
  return footage_;
}

Project *FootageViewerWidget::GetTimelinePointsProject()
{
  return footage_->project();
}

void FootageViewerWidget::StartFootageDragInternal(bool enable_video, bool enable_audio)
{
  if (!GetFootage()) {
    return;
  }

  QDrag* drag = new QDrag(this);
  QMimeData* mimedata = new QMimeData();

  QByteArray encoded_data;
  QDataStream stream(&encoded_data, QIODevice::WriteOnly);

  quint64 enabled_stream_flags = GetFootage()->get_enabled_stream_flags();

  // Disable streams that have been disabled
  if (!enable_video || !enable_audio) {
    quint64 stream_disabler = 0x1;

    foreach (StreamPtr s, GetFootage()->streams()) {
      if ((s->type() == Stream::kVideo && !enable_video)
          || (s->type() == Stream::kAudio && !enable_audio)) {
        enabled_stream_flags &= ~stream_disabler;
      }

      stream_disabler <<= 1;
    }
  }

  stream << enabled_stream_flags << -1 << reinterpret_cast<quintptr>(GetFootage());

  mimedata->setData("application/x-oliveprojectitemdata", encoded_data);
  drag->setMimeData(mimedata);

  drag->exec();
}

void FootageViewerWidget::StartFootageDrag()
{
  StartFootageDragInternal(true, true);
}

void FootageViewerWidget::StartVideoDrag()
{
  StartFootageDragInternal(true, false);
}

void FootageViewerWidget::StartAudioDrag()
{
  StartFootageDragInternal(false, true);
}

OLIVE_NAMESPACE_EXIT

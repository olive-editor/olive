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

#include "config/config.h"
#include "project/project.h"

OLIVE_NAMESPACE_ENTER

FootageViewerWidget::FootageViewerWidget(QWidget *parent) :
  ViewerWidget(parent),
  footage_(nullptr)
{
  video_node_ = new VideoInput();
  audio_node_ = new AudioInput();
  viewer_node_ = new ViewerOutput();

  connect(display_widget(), &ViewerDisplayWidget::DragStarted, this, &FootageViewerWidget::StartFootageDrag);

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
    cached_timestamps_.insert(footage_, GetTimestamp());

    ConnectViewerNode(nullptr);

    NodeParam::DisconnectEdge(video_node_->output(), viewer_node_->texture_input());
    NodeParam::DisconnectEdge(audio_node_->output(), viewer_node_->samples_input());
  }

  footage_ = footage;

  if (footage_) {
    VideoStreamPtr video_stream = nullptr;
    AudioStreamPtr audio_stream = nullptr;

    foreach (StreamPtr s, footage_->streams()) {
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

    viewer_node_->set_media_name(footage_->name());

    if (video_stream) {
      video_node_->SetFootage(video_stream);
      viewer_node_->set_video_params(VideoParams(video_stream->width(),
                                                 video_stream->height(),
                                                 video_stream->frame_rate().flipped(),
                                                 static_cast<PixelFormat::Format>(Config::Current()["DefaultSequencePreviewFormat"].toInt()),
                                                 VideoParams::generate_auto_divider(video_stream->width(), video_stream->height())));
      NodeParam::ConnectEdge(video_node_->output(), viewer_node_->texture_input());
    } else {
      int width = Config::Current()["DefaultSequenceWidth"].toInt();
      int height = Config::Current()["DefaultSequenceHeight"].toInt();

      viewer_node_->set_video_params(VideoParams(width,
                                                 height,
                                                 Config::Current()["DefaultSequenceFrameRate"].value<rational>(),
                                                 static_cast<PixelFormat::Format>(Config::Current()["DefaultSequencePreviewFormat"].toInt()),
                                                 VideoParams::generate_auto_divider(width, height)));
    }

    if (audio_stream) {
      audio_node_->SetFootage(audio_stream);
      viewer_node_->set_audio_params(AudioParams(audio_stream->sample_rate(), audio_stream->channel_layout(), SampleFormat::kInternalFormat));
      NodeParam::ConnectEdge(audio_node_->output(), viewer_node_->samples_input());
    } else {
      viewer_node_->set_audio_params(AudioParams(Config::Current()["DefaultSequenceAudioFrequency"].toInt(),
                                     Config::Current()["DefaultSequenceAudioLayout"].toULongLong(),
                                     SampleFormat::kInternalFormat));
    }

    ConnectViewerNode(viewer_node_, footage_->project()->color_manager());

    SetTimestamp(cached_timestamps_.value(footage_, 0));
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

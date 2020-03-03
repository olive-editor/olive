#include "footageviewer.h"

#include <QDrag>
#include <QMimeData>

#include "project/project.h"

FootageViewerWidget::FootageViewerWidget(QWidget *parent) :
  ViewerWidget(parent),
  footage_(nullptr)
{
  video_node_ = new VideoInput();
  audio_node_ = new AudioInput();
  viewer_node_ = new ViewerOutput();

  NodeParam::ConnectEdge(video_node_->output(), viewer_node_->texture_input());
  NodeParam::ConnectEdge(audio_node_->output(), viewer_node_->samples_input());
  NodeParam::ConnectEdge(video_node_->output(), viewer_node_->length_input());

  connect(gl_widget_, &ViewerGLWidget::DragStarted, this, &FootageViewerWidget::StartFootageDrag);
}

Footage *FootageViewerWidget::GetFootage() const
{
  return footage_;
}

void FootageViewerWidget::SetFootage(Footage *footage)
{
  if (footage_) {
    ConnectViewerNode(nullptr);
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
    }

    if (audio_stream) {
      audio_node_->SetFootage(audio_stream);
      viewer_node_->set_audio_params(AudioParams(audio_stream->sample_rate(), audio_stream->channel_layout()));
    }

    ConnectViewerNode(viewer_node_, footage->project()->color_manager());
    video_renderer_->InvalidateCache(TimeRange(0, viewer_node_->Length()));
    audio_renderer_->InvalidateCache(TimeRange(0, viewer_node_->Length()));
  }
}

void FootageViewerWidget::StartFootageDrag()
{
  if (!GetFootage()) {
    return;
  }

  qDebug() << "Drag start!";
  QDrag* drag = new QDrag(this);
  QMimeData* mimedata = new QMimeData();

  QByteArray encoded_data;
  QDataStream stream(&encoded_data, QIODevice::WriteOnly);

  stream << -1 << reinterpret_cast<quintptr>(GetFootage());

  mimedata->setData("application/x-oliveprojectitemdata", encoded_data);
  drag->setMimeData(mimedata);

  drag->exec();
}

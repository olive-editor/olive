/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

FootageViewerWidget::FootageViewerWidget(QWidget *parent) :
  ViewerWidget(parent),
  footage_(nullptr)
{
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

    Node::DisconnectEdge(sequence_.GetConnectedOutput(ViewerOutput::kTextureInput), NodeInput(&sequence_, ViewerOutput::kTextureInput));
    Node::DisconnectEdge(sequence_.GetConnectedOutput(ViewerOutput::kSamplesInput), NodeInput(&sequence_, ViewerOutput::kSamplesInput));
  }

  footage_ = footage;

  if (footage_) {
    // Update sequence media name
    sequence_.SetLabel(footage_->GetLabel());

    // Reset parameters and then attempt to set from footage
    sequence_.set_default_parameters();
    sequence_.set_parameters_from_footage({footage_});

    // Try to connect video stream
    TryConnectingType(footage, Stream::kVideo);
    TryConnectingType(footage, Stream::kAudio);

    ConnectViewerNode(&sequence_, footage_->project()->color_manager());

    SetTimestamp(cached_timestamps_.value(footage_, 0));
  } else {
    SetTimestamp(0);
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
  QDataStream data_stream(&encoded_data, QIODevice::WriteOnly);

  quint64 enabled_stream_flags = GetFootage()->get_enabled_stream_flags();

  // Disable streams that have been disabled
  if (!enable_video || !enable_audio) {
    quint64 stream_disabler = 0x1;

    for (int i=0; i<GetFootage()->GetStreamCount(); i++) {
      Stream stream = GetFootage()->GetStreamAt(i);

      if ((stream.type() == Stream::kVideo && !enable_video)
          || (stream.type() == Stream::kAudio && !enable_audio)) {
        enabled_stream_flags &= ~stream_disabler;
      }

      stream_disabler <<= 1;
    }
  }

  data_stream << enabled_stream_flags << -1 << reinterpret_cast<quintptr>(GetFootage());

  mimedata->setData("application/x-oliveprojectitemdata", encoded_data);
  drag->setMimeData(mimedata);

  drag->exec();
}

void FootageViewerWidget::TryConnectingType(Footage *footage, Stream::Type type)
{
  for (int i=0; ; i++) {
    Stream stream = footage_->GetStreamAt(type, i);

    // Found end of stream list
    if (!stream.IsValid()) {
      break;
    }

    if (stream.enabled()) {
      QString s = Footage::GetStringFromReference(type, i);

      QString input_param;

      if (type == Stream::kVideo) {
        input_param = ViewerOutput::kTextureInput;
      } else {
        input_param = ViewerOutput::kSamplesInput;
      }

      Node::ConnectEdge(NodeOutput(footage, s), NodeInput(&sequence_, input_param));
    }
  }
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

}

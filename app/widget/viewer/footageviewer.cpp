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
  if (footage && !footage->project()) {
    qCritical() << "Failed to set footage in footage viewer - footage had no project";
    return;
  }

  if (footage_) {
    cached_timestamps_.insert(footage_, GetTimestamp());

    ViewerOutput* old_viewer = footage_->project()->footage_viewer();

    ConnectViewerNode(nullptr);

    if (old_viewer->IsInputConnected(ViewerOutput::kTextureInput)) {
      Node::DisconnectEdge(old_viewer->GetConnectedOutput(ViewerOutput::kTextureInput), NodeInput(old_viewer, ViewerOutput::kTextureInput));
    }

    if (old_viewer->IsInputConnected(ViewerOutput::kSamplesInput)) {
      Node::DisconnectEdge(old_viewer->GetConnectedOutput(ViewerOutput::kSamplesInput), NodeInput(old_viewer, ViewerOutput::kSamplesInput));
    }
  }

  footage_ = footage;

  if (footage_) {
    ViewerOutput* new_viewer = footage_->project()->footage_viewer();

    // Update sequence media name
    new_viewer->SetLabel(footage_->GetLabel());

    // Reset parameters and then attempt to set from footage
    new_viewer->set_default_parameters();
    new_viewer->set_parameters_from_footage({footage_});

    // Try to connect video stream
    TryConnectingType(new_viewer, footage, Stream::kVideo);
    TryConnectingType(new_viewer, footage, Stream::kAudio);

    ConnectViewerNode(new_viewer);

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

  QVector<Footage::StreamReference> streams = GetFootage()->GetEnabledStreamsAsReferences();

  // Disable streams that have been disabled
  if (!enable_video || !enable_audio) {
    for (int i=0; i<streams.size(); i++) {
      const Footage::StreamReference& ref = streams.at(i);

      if ((ref.type() == Stream::kVideo && !enable_video)
          || (ref.type() == Stream::kAudio && !enable_audio)) {
        streams.removeAt(i);
        i--;
      }
    }
  }

  if (!streams.isEmpty()) {
    data_stream << streams << reinterpret_cast<quintptr>(GetFootage());

    mimedata->setData(QStringLiteral("application/x-oliveprojectitemdata"), encoded_data);
    drag->setMimeData(mimedata);

    drag->exec();
  }
}

void FootageViewerWidget::TryConnectingType(ViewerOutput* viewer, Footage *footage, Stream::Type type)
{
  int index = -1;

  for (int i=0; ; i++) {
    if (type == Stream::kVideo) {
      VideoParams vp = footage->GetVideoParams(i);

      if (!vp.is_valid()) {
        break;
      }

      if (vp.enabled()) {
        index = i;
        break;
      }
    } else if (type == Stream::kAudio) {
      AudioParams vp = footage->GetAudioParams(i);

      if (!vp.is_valid()) {
        break;
      }

      if (vp.enabled()) {
        index = i;
        break;
      }
    }
  }

  if (index != -1) {
    QString s = Footage::GetStringFromReference(type, index);

    QString input_param;

    if (type == Stream::kVideo) {
      input_param = ViewerOutput::kTextureInput;
    } else {
      input_param = ViewerOutput::kSamplesInput;
    }

    Node::ConnectEdge(NodeOutput(footage, s), NodeInput(viewer, input_param));
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

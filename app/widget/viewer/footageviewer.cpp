/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "node/project/project.h"

namespace olive {

#define super ViewerWidget

FootageViewerWidget::FootageViewerWidget(QWidget *parent) :
  super(parent)
{
  connect(display_widget(), &ViewerDisplayWidget::DragStarted, this, &FootageViewerWidget::StartFootageDrag);

  controls_->SetAudioVideoDragButtonsVisible(true);
  connect(controls_, &PlaybackControls::VideoPressed, this, &FootageViewerWidget::StartVideoDrag);
  connect(controls_, &PlaybackControls::AudioPressed, this, &FootageViewerWidget::StartAudioDrag);
}

void FootageViewerWidget::ConnectNodeEvent(ViewerOutput *n)
{
  super::ConnectNodeEvent(n);

  SetTimestamp(cached_timestamps_.value(n, 0));
}

void FootageViewerWidget::DisconnectNodeEvent(ViewerOutput *n)
{
  // Cache timestamp in case this footage is opened again later
  cached_timestamps_.insert(n, GetTimestamp());
  SetTimestamp(0);

  super::DisconnectNodeEvent(n);
}

void FootageViewerWidget::StartFootageDragInternal(bool enable_video, bool enable_audio)
{
  if (!GetConnectedNode()) {
    return;
  }

  QDrag* drag = new QDrag(this);
  QMimeData* mimedata = new QMimeData();

  QByteArray encoded_data;
  QDataStream data_stream(&encoded_data, QIODevice::WriteOnly);

  QVector<Track::Reference> streams = GetConnectedNode()->GetEnabledStreamsAsReferences();

  // Disable streams that have been disabled
  if (!enable_video || !enable_audio) {
    for (int i=0; i<streams.size(); i++) {
      const Track::Reference& ref = streams.at(i);

      if ((ref.type() == Track::kVideo && !enable_video)
          || (ref.type() == Track::kAudio && !enable_audio)) {
        streams.removeAt(i);
        i--;
      }
    }
  }

  if (!streams.isEmpty()) {
    data_stream << streams << reinterpret_cast<quintptr>(GetConnectedNode());

    mimedata->setData(QStringLiteral("application/x-oliveprojectitemdata"), encoded_data);
    drag->setMimeData(mimedata);

    drag->exec();
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

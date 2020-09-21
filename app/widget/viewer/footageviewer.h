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

#ifndef FOOTAGEVIEWERWIDGET_H
#define FOOTAGEVIEWERWIDGET_H

#include "node/input/media/audio/audio.h"
#include "node/input/media/video/video.h"
#include "node/output/viewer/viewer.h"
#include "viewer.h"

OLIVE_NAMESPACE_ENTER

class FootageViewerWidget : public ViewerWidget
{
  Q_OBJECT
public:
  FootageViewerWidget(QWidget* parent = nullptr);

  Footage* GetFootage() const;
  void SetFootage(Footage* footage);

protected:
  virtual TimelinePoints* ConnectTimelinePoints() override;

  virtual Project* GetTimelinePointsProject() override;

private:
  void StartFootageDragInternal(bool enable_video, bool enable_audio);

  Footage* footage_;

  Sequence sequence_;

  VideoInput* video_node_;

  AudioInput* audio_node_;

  QHash<Footage*, int64_t> cached_timestamps_;

private slots:
  void StartFootageDrag();

  void StartVideoDrag();

  void StartAudioDrag();

};

OLIVE_NAMESPACE_EXIT

#endif // FOOTAGEVIEWERWIDGET_H

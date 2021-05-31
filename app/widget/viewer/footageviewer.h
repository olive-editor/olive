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

#ifndef FOOTAGEVIEWERWIDGET_H
#define FOOTAGEVIEWERWIDGET_H

#include "node/output/viewer/viewer.h"
#include "viewer.h"

namespace olive {

class FootageViewerWidget : public ViewerWidget
{
  Q_OBJECT
public:
  FootageViewerWidget(QWidget* parent = nullptr);

protected:
  virtual void ConnectNodeEvent(ViewerOutput *) override;

  virtual void DisconnectNodeEvent(ViewerOutput *) override;

private:
  void StartFootageDragInternal(bool enable_video, bool enable_audio);

  QHash<ViewerOutput*, int64_t> cached_timestamps_;

private slots:
  void StartFootageDrag();

  void StartVideoDrag();

  void StartAudioDrag();

};

}

#endif // FOOTAGEVIEWERWIDGET_H

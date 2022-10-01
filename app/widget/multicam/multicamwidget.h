/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef MULTICAMWIDGET_H
#define MULTICAMWIDGET_H

#include "multicamdisplay.h"
#include "node/input/multicam/multicamnode.h"
#include "widget/viewer/viewer.h"

namespace olive {

class MulticamWidget : public TimeBasedWidget
{
  Q_OBJECT
public:
  explicit MulticamWidget(QWidget *parent = nullptr);

  MulticamDisplay *GetDisplayWidget() const { return display_; }

  void SetMulticamNode(MultiCamNode *n);

  void SetClip(ClipBlock *clip);

protected:
  virtual void ConnectNodeEvent(ViewerOutput *n) override;
  virtual void DisconnectNodeEvent(ViewerOutput *n) override;

private:
  void Switch(int source, bool split_clip);

  ViewerSizer *sizer_;

  MulticamDisplay *display_;

  MultiCamNode *node_;

  ClipBlock *clip_;

private slots:
  void DisplayClicked(const QPoint &p);

};

}

#endif // MULTICAMWIDGET_H

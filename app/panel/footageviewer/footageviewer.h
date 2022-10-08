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

#ifndef FOOTAGE_VIEWER_PANEL_H
#define FOOTAGE_VIEWER_PANEL_H

#include <QOpenGLFunctions>

#include "panel/viewer/viewerbase.h"
#include "panel/project/footagemanagementpanel.h"
#include "widget/viewer/footageviewer.h"

namespace olive {

/**
 * @brief Dockable wrapper around a ViewerWidget
 */
class FootageViewerPanel : public ViewerPanelBase, public FootageManagementPanel {
  Q_OBJECT
public:
  FootageViewerPanel(QWidget* parent);

  void OverrideWorkArea(const TimeRange &r);

  FootageViewerWidget *GetFootageViewerWidget() const
  {
    return static_cast<FootageViewerWidget*>(GetTimeBasedWidget());
  }

  virtual QVector<ViewerOutput *> GetSelectedFootage() const override;

protected:
  virtual void Retranslate() override;

};

}

#endif // FOOTAGE_VIEWER_PANEL_H

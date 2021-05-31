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

#ifndef VIEWER_PANEL_H
#define VIEWER_PANEL_H

#include <QOpenGLFunctions>

#include "viewerbase.h"

namespace olive {

/**
 * @brief Dockable wrapper around a ViewerWidget
 */
class ViewerPanel : public ViewerPanelBase {
  Q_OBJECT
public:
  ViewerPanel(const QString& object_name, QWidget* parent);
  ViewerPanel(QWidget* parent);

protected:
  virtual void Retranslate() override;

private:
  void Init();

};

}

#endif // VIEWER_PANEL_H

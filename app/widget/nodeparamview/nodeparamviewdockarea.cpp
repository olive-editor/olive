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

#include "nodeparamviewdockarea.h"

#include <QDockWidget>

namespace olive {

NodeParamViewDockArea::NodeParamViewDockArea(QWidget *parent) :
  QMainWindow(parent)
{
  // Disable dock widgets from tabbing and disable glitchy animations
  setDockOptions(static_cast<QMainWindow::DockOption>(0));

  // HACK: Hide the main window separators (unfortunately the cursors still appear)
  setStyleSheet(QStringLiteral("QMainWindow::separator {background: rgba(0, 0, 0, 0)}"));
}

QMenu *NodeParamViewDockArea::createPopupMenu()
{
  return nullptr;
}

void NodeParamViewDockArea::AddItem(QDockWidget *item)
{
  item->setAllowedAreas(Qt::LeftDockWidgetArea);
  item->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  addDockWidget(Qt::LeftDockWidgetArea, item);
}

}

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

#ifndef NODEPARAMVIEWDOCKAREA_H
#define NODEPARAMVIEWDOCKAREA_H

#include <QMainWindow>

namespace olive {

// This may look weird, but QMainWindow is just a QWidget with a fancy layout that allows
// for docking QDockWidgets
class NodeParamViewDockArea : public QMainWindow
{
  Q_OBJECT
public:
  explicit NodeParamViewDockArea(QWidget *parent = nullptr);

  virtual QMenu *createPopupMenu() override;

  void AddItem(QDockWidget *item);

};

}

#endif // NODEPARAMVIEWDOCKAREA_H

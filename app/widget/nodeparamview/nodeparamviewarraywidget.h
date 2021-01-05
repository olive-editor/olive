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

#ifndef NODEPARAMVIEWARRAYWIDGET_H
#define NODEPARAMVIEWARRAYWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "node/input.h"

namespace olive {

class NodeParamViewArrayWidget : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewArrayWidget(NodeInput* array, QWidget* parent = nullptr);

private:
  NodeInput* array_;

  QLabel* count_lbl_;

  QPushButton* plus_btn_;

private slots:
  void UpdateCounter();

  void AddElement();

};

}

#endif // NODEPARAMVIEWARRAYWIDGET_H

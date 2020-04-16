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

#ifndef NODEPARAMVIEWCONNECTEDLABEL_H
#define NODEPARAMVIEWCONNECTEDLABEL_H

#include "node/input.h"
#include "widget/clickablelabel/clickablelabel.h"

OLIVE_NAMESPACE_ENTER

class NodeParamViewConnectedLabel : public QWidget {
  Q_OBJECT
public:
  NodeParamViewConnectedLabel(NodeInput* input, QWidget* parent = nullptr);

signals:
  void ConnectionClicked();

private slots:
  void UpdateConnected();

private:
  ClickableLabel* connected_to_lbl_;

  NodeInput* input_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWCONNECTEDLABEL_H

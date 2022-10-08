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

#ifndef NODEPARAMVIEWCONNECTEDLABEL_H
#define NODEPARAMVIEWCONNECTEDLABEL_H

#include "node/param.h"
#include "widget/clickablelabel/clickablelabel.h"
#include "widget/nodevaluetree/nodevaluetree.h"

namespace olive {

class NodeParamViewConnectedLabel : public QWidget {
  Q_OBJECT
public:
  NodeParamViewConnectedLabel(const NodeInput& input, QWidget* parent = nullptr);

  void SetTime(const rational &time);

signals:
  void RequestSelectNode(Node *n);

private slots:
  void InputConnected(Node *output, const NodeInput &input);

  void InputDisconnected(Node *output, const NodeInput &input);

  void ShowLabelContextMenu();

  void ConnectionClicked();

private:
  void UpdateLabel();

  void UpdateValueTree();

  void CreateTree();

  ClickableLabel* connected_to_lbl_;

  NodeInput input_;

  Node *connected_node_;

  NodeValueTree *value_tree_;

  rational time_;

private slots:
  void SetValueTreeVisible(bool e);

};

}

#endif // NODEPARAMVIEWCONNECTEDLABEL_H

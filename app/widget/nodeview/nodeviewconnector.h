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

#ifndef NODEVIEWCONNECTOR_H
#define NODEVIEWCONNECTOR_H

#include <QGraphicsItem>

#include "nodeviewcommon.h"
#include "node/node.h"

namespace olive {

class NodeViewConnector : public QGraphicsItem
{
public:
  NodeViewConnector(QGraphicsItem *parent = nullptr);

  virtual QRectF boundingRect() const override
  {
    return rect_;
  }

  const QString &GetParam() const
  {
    return param_;
  }

  bool IsInput() const
  {
    return is_input_;
  }

  Node *GetNode() const
  {
    return node_;
  }

  void SetParam(Node *node, const QString &param, bool is_input)
  {
    node_ = node;
    param_ = param;
    is_input_ = is_input;
  }

  void SetFlowDirection(NodeViewCommon::FlowDirection flow_dir)
  {
    flow_dir_ = flow_dir;

    update();
  }

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  Node *node_;

  bool is_input_;

  QString param_;

  NodeViewCommon::FlowDirection flow_dir_;

  QRectF rect_;

};

}

#endif // NODEVIEWCONNECTOR_H

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

#ifndef NODEEDGEITEM_H
#define NODEEDGEITEM_H

#include <QGraphicsPathItem>
#include <QPalette>

#include "curvedconnectoritem.h"
#include "node/node.h"

namespace olive {

class NodeViewItem;

/**
 * @brief A graphical representation of a NodeEdge to be used in NodeView
 *
 * A fairly simple line widget use to visualize a connection between two node parameters (a NodeEdge).
 */
class NodeViewEdge : public CurvedConnectorItem
{
public:
  NodeViewEdge(const NodeOutput &output, const NodeInput& input,
               NodeViewItem* from_item, NodeViewItem* to_item,
               QGraphicsItem* parent = nullptr);

  NodeViewEdge(QGraphicsItem* parent = nullptr);

  virtual ~NodeViewEdge() override;

  const NodeOutput &output() const { return output_; }
  const NodeInput &input() const { return input_; }
  int element() const { return element_; }
  NodeViewItem* from_item() const { return from_item_; }
  NodeViewItem* to_item() const { return to_item_; }

  void set_from_item(NodeViewItem *i);
  void set_to_item(NodeViewItem *i);

  void Adjust();

protected:
  virtual NodeViewCommon::FlowDirection GetFromDirection() const override;
  virtual NodeViewCommon::FlowDirection GetToDirection() const override;

private:
  NodeOutput output_;

  NodeInput input_;

  int element_;

  NodeViewItem* from_item_;

  NodeViewItem* to_item_;

};

}

#endif // NODEEDGEITEM_H

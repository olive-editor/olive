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

#include "nodeviewedge.h"

#include <QApplication>
#include <QDebug>

#include "nodeview.h"
#include "nodeviewitem.h"
#include "nodeviewscene.h"

namespace olive {

#define super CurvedConnectorItem

NodeViewEdge::NodeViewEdge(QGraphicsItem *parent) :
  super(parent),
  from_item_(nullptr),
  to_item_(nullptr)
{
}

NodeViewEdge::NodeViewEdge(const NodeOutput &output, const NodeInput &input,
                           NodeViewItem* from_item, NodeViewItem* to_item,
                           QGraphicsItem* parent) :
  NodeViewEdge(parent)
{
  output_ = output;
  input_ = input;
  from_item_ = from_item;
  to_item_ = to_item;

  SetConnected(true);

  from_item_->AddEdge(this);
  to_item_->AddEdge(this);
}

NodeViewEdge::~NodeViewEdge()
{
  if (from_item_) {
    from_item_->RemoveEdge(this);
  }

  if (to_item_) {
    to_item_->RemoveEdge(this);
  }
}

void NodeViewEdge::set_from_item(NodeViewItem *i)
{
  if (from_item_) {
    from_item_->RemoveEdge(this);
  }

  from_item_ = i;

  if (from_item_) {
    from_item_->AddEdge(this);
  }

  Adjust();
}

void NodeViewEdge::set_to_item(NodeViewItem *i)
{
  if (to_item_) {
    to_item_->RemoveEdge(this);
  }

  to_item_ = i;

  if (to_item_) {
    to_item_->AddEdge(this);
  }

  Adjust();
}

void NodeViewEdge::Adjust()
{
  // Draw a line between the two
  SetPoints(from_item()->GetOutputPoint(), to_item()->GetInputPoint());
}

NodeViewCommon::FlowDirection NodeViewEdge::GetFromDirection() const
{
  return from_item_ ? from_item_->GetFlowDirection() : NodeViewCommon::kInvalidDirection;
}

NodeViewCommon::FlowDirection NodeViewEdge::GetToDirection() const
{
  return to_item_ ? to_item_->GetFlowDirection() : NodeViewCommon::kInvalidDirection;
}

}

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

#include "nodeparamview.h"

NodeParamView::NodeParamView(QWidget *parent) :
  QWidget(parent)
{
  // Set new layout
  layout_ = new QVBoxLayout(this);
  layout_->setSpacing(0);
  layout_->setMargin(0);

  // Add a stretch to allow empty space at the bottom of the layout
  layout_->addStretch();
}

void NodeParamView::SetNodes(QList<Node *> nodes)
{
  // If we already have item widgets, delete them all now
  foreach (NodeParamViewItem* item, items_) {
    delete item;
  }
  items_.clear();

  // Set the internal list to the one we've received
  nodes_ = nodes;

  // For each node, create a widget
  foreach (Node* node, nodes_) {

    // See if we already have an item for this node type

    bool merged_node = false;

    foreach (NodeParamViewItem* existing_item, items_) {
      if (existing_item->CanAddNode(node)) {
        existing_item->AttachNode(node);
        merged_node = true;
        break;
      }
    }

    // If we couldn't merge this node into the existing item, create a new one
    if (!merged_node) {
      NodeParamViewItem* item = new NodeParamViewItem(this);

      item->AttachNode(node);

      // Insert the widget before the stretch
      layout_->insertWidget(layout_->count() - 1, item);

      items_.append(item);
    }
  }
}

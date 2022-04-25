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

#include "nodeparamviewcontext.h"

#include <QMessageBox>

#include "node/block/clip/clip.h"

namespace olive {

#define super NodeParamViewItemBase

NodeParamViewContext::NodeParamViewContext(QWidget *parent) :
  super(parent)
{
  QWidget *body = new QWidget();
  QHBoxLayout *body_layout = new QHBoxLayout(body);
  SetBody(body);

  dock_area_ = new NodeParamViewDockArea();
  body_layout->addWidget(dock_area_);

  setBackgroundRole(QPalette::Base);

  Retranslate();

  connect(title_bar(), &NodeParamViewItemTitleBar::AddEffectButtonClicked, this, &NodeParamViewContext::AddEffectButtonClicked);
}

NodeParamViewItem *NodeParamViewContext::GetItem(Node *node, Node *ctx)
{
  for (auto it=items_.begin(); it!=items_.end(); it++) {
    NodeParamViewItem *item = *it;

    if (item->GetNode() == node && item->GetContext() == ctx) {
      return item;
    }
  }

  return nullptr;
}

void NodeParamViewContext::AddNode(NodeParamViewItem *item)
{
  items_.append(item);
  dock_area_->AddItem(item);
}

void NodeParamViewContext::RemoveNode(Node *node, Node *ctx)
{
  for (auto it=items_.begin(); it!=items_.end(); ) {
    NodeParamViewItem *item = *it;

    if (item->GetNode() == node && item->GetContext() == ctx) {
      emit AboutToDeleteItem(item);
      delete item;
      it = items_.erase(it);
    } else {
      it++;
    }
  }
}

void NodeParamViewContext::RemoveNodesWithContext(Node *ctx)
{
  for (auto it=items_.begin(); it!=items_.end(); ) {
    NodeParamViewItem *item = *it;

    if (item->GetContext() == ctx) {
      emit AboutToDeleteItem(item);
      delete item;
      it = items_.erase(it);
    } else {
      it++;
    }
  }
}

void NodeParamViewContext::SetInputChecked(const NodeInput &input, bool e)
{
  foreach (NodeParamViewItem *item, items_) {
    if (item->GetNode() == input.node()) {
      item->SetInputChecked(input, e);
    }
  }
}

void NodeParamViewContext::SetTimebase(const rational &timebase)
{
  foreach (NodeParamViewItem* item, items_) {
    item->SetTimebase(timebase);
  }
}

void NodeParamViewContext::SetTimeTarget(Node *n)
{
  foreach (NodeParamViewItem* item, items_) {
    item->SetTimeTarget(n);
  }
}

void NodeParamViewContext::SetTime(const rational &time)
{
  foreach (NodeParamViewItem* item, items_) {
    item->SetTime(time);
  }
}

void NodeParamViewContext::Retranslate()
{
}

void NodeParamViewContext::AddEffectButtonClicked()
{
  QMessageBox::information(this, tr("STUB"), tr("This feature is coming soon. Thanks for testing development builds of Olive :)"));
}

}

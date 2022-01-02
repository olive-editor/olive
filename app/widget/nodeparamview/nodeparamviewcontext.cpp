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

void NodeParamViewContext::AddNode(NodeParamViewItem *item)
{
  items_.insert(item->GetNode(), item);
  dock_area_->AddItem(item);
}

void NodeParamViewContext::RemoveNode(Node *node)
{
}

void NodeParamViewContext::Clear()
{
  qDeleteAll(items_);
  items_.clear();
}

void NodeParamViewContext::SetInputChecked(const NodeInput &input, bool e)
{
  if (NodeParamViewItem *item = items_.value(input.node())) {
    item->SetInputChecked(input, e);
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

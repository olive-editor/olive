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

#include "nodeparamviewcontext.h"

#include <QMessageBox>

#include "node/block/clip/clip.h"
#include "node/factory.h"
#include "node/nodeundo.h"

namespace olive {

#define super NodeParamViewItemBase

NodeParamViewContext::NodeParamViewContext(QWidget *parent) :
  super(parent),
  type_(Track::kNone)
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

void NodeParamViewContext::SetTimeTarget(ViewerOutput *n)
{
  foreach (NodeParamViewItem* item, items_) {
    item->SetTimeTarget(n);
  }
}

void NodeParamViewContext::SetEffectType(Track::Type type)
{
  type_ = type;
}

void NodeParamViewContext::Retranslate()
{
}

void NodeParamViewContext::AddEffectButtonClicked()
{
  Node::Flag flag = Node::kNone;

  if (type_ == Track::kVideo) {
    flag = Node::kVideoEffect;
  } else {
    flag = Node::kAudioEffect;
  }

  if (flag == Node::kNone) {
    return;
  }

  Menu *m = NodeFactory::CreateMenu(this, false, Node::kCategoryUnknown, flag);
  connect(m, &Menu::triggered, this, &NodeParamViewContext::AddEffectMenuItemTriggered);
  m->exec(QCursor::pos());
  delete m;
}

void NodeParamViewContext::AddEffectMenuItemTriggered(QAction *a)
{
  Node *n = NodeFactory::CreateFromMenuAction(a);

  if (n) {
    NodeInput new_node_input = n->GetEffectInput();
    MultiUndoCommand *command = new MultiUndoCommand();

    QVector<Project*> graphs_added_to;

    foreach (Node *ctx, contexts_) {
      NodeInput ctx_input = ctx->GetEffectInput();

      if (!graphs_added_to.contains(ctx->parent())) {
        command->add_child(new NodeAddCommand(ctx->parent(), n));
        graphs_added_to.append(ctx->parent());
      }

      command->add_child(new NodeSetPositionCommand(n, ctx, ctx->GetNodePositionInContext(ctx)));
      command->add_child(new NodeSetPositionCommand(ctx, ctx, ctx->GetNodePositionInContext(ctx) + QPointF(1, 0)));

      if (ctx_input.IsConnected()) {
        Node *prev_output = ctx_input.GetConnectedOutput();

        command->add_child(new NodeEdgeRemoveCommand(prev_output, ctx_input));
        command->add_child(new NodeEdgeAddCommand(prev_output, new_node_input));
      }

      command->add_child(new NodeEdgeAddCommand(n, ctx_input));
    }

    Core::instance()->undo_stack()->push(command, tr("Added %1 to Node Chain").arg(n->Name()));
  }
}

}

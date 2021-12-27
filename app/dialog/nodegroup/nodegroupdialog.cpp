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

#include "nodegroupdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>

#include "panel/node/node.h"
#include "panel/panelmanager.h"
#include "panel/param/param.h"

namespace olive {

#define super QDialog

NodeGroupDialog::NodeGroupDialog(NodeGroup *group, QWidget *parent) :
  super(parent),
  group_(group),
  prepend_undo_(nullptr)
{
  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Name:")), row, 0);

  name_edit_ = new QLineEdit();
  name_edit_->setText(group->GetLabel());
  layout->addWidget(name_edit_, row, 1);

  row++;

  QMainWindow *splitter = new QMainWindow();
  QWidget *cw = new QWidget();
  cw->setFixedSize(0, 0);
  splitter->setCentralWidget(cw);
  layout->addWidget(splitter, row, 0, 1, 2);

  ParamPanel *param_view = PanelManager::instance()->CreatePanel<ParamPanel>(splitter);
  param_view->GetParamView()->SetIgnoreNodeFlags(true);
  param_view->GetParamView()->SetCreateCheckBoxes(kCheckBoxesOnNonConnected);
  splitter->addDockWidget(Qt::LeftDockWidgetArea, param_view);

  NodePanel *node_view = PanelManager::instance()->CreatePanel<NodePanel>(splitter);
  node_view->GetNodeWidget()->view()->OverrideUndoStack(&undo_stack_);
  QMetaObject::invokeMethod(node_view->GetNodeWidget()->view(), &NodeView::CenterOnItemsBoundingRect, Qt::QueuedConnection);
  splitter->addDockWidget(Qt::RightDockWidgetArea, node_view);

  row++;

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btns->setCenterButtons(true);
  connect(btns, &QDialogButtonBox::accepted, this, &NodeGroupDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &NodeGroupDialog::reject);
  layout->addWidget(btns, row, 0, 1, 2);

  setWindowTitle(tr("Group Editor - %1").arg(group->GetLabelOrName()));

  // Create a project
  copied_project_ = new Project();
  copied_project_->setParent(this);

  // Copy project nodes
  // NOTE: I hate this. What might be better is to fold the "ColorManager" and "ProjectSettings"
  //       nodes into "Project" inputs and make "Project" a node too.
  for (int i=0; i<copied_project_->nodes().size(); i++) {
    Node *ours = copied_project_->nodes().at(i);
    Node *theirs = group_->project()->nodes().at(i);

    copy_subnodes_.insert(theirs, ours);
    Node::CopyInputs(ours, theirs, false);
  }

  // Copy group and nodes
  copy_group_ = static_cast<NodeGroup*>(group_->copy());
  copy_group_->setParent(copied_project_);

  // Copy subnodes with positions
  const Node::PositionMap &map = group_->GetContextPositions();
  for (auto it=map.cbegin(); it!=map.cend(); it++) {
    Node *copy = it.key()->copy();
    copy->SetUUID(it.key()->GetUUID());
    copy->setParent(copied_project_);

    copy_group_->SetNodePositionInContext(copy, it.value());
    copy_subnodes_.insert(it.key(), copy);
  }

  for (auto it=map.cbegin(); it!=map.cend(); it++) {
    Node::CopyInputs(it.key(), copy_subnodes_.value(it.key()), false);
  }

  // Copy edges
  for (auto it=copy_subnodes_.cbegin(); it!=copy_subnodes_.cend(); it++) {
    Node *src = it.key();
    Node *cpy = it.value();

    for (auto jt=src->input_connections().cbegin(); jt!=src->input_connections().cend(); jt++) {
      if (Node *cpy_output = copy_subnodes_.value(jt->second)) {
        Node::ConnectEdge(cpy_output, NodeInput(cpy, jt->first.input(), jt->first.element()));
        copied_edges_.append({cpy_output, NodeInput(cpy, jt->first.input(), jt->first.element())});
      }
    }
  }

  node_view->SetContexts({copy_group_});

  for (auto it=group_->GetInputPassthroughs().cbegin(); it!=group_->GetInputPassthroughs().cend(); it++) {
    const NodeInput &src_input = it.value();
    NodeInput copy_input(copy_subnodes_.value(src_input.node()), src_input.input(), src_input.element());
    param_view->GetParamView()->SetInputChecked(copy_input, true);
  }

  param_view->SetContexts({copy_group_});
}

void NodeGroupDialog::accept()
{
  // First, validate if a connection, deleted node, or disabled passthrough is going to disconnect
  // something elsewhere in the group. Ask the user to confirm if so.

  // Detect removed nodes
  QVector<Node*> nodes_to_delete = GetNodesToDelete();

  // Detect new connections
  QVector<Node::OutputConnection> edges_to_connect = GetNewConnections();

  // Warn user if operation will disconnect a node outside the group
  if (OperationWillAffectOutsideGroup(nodes_to_delete, edges_to_connect)) {
    if (QMessageBox::question(this, QString(), tr("This operation will disconnect nodes outside of this group. Do you wish to continue?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
      return;
    }
  }

  NodeViewDeleteCommand *delete_command = new NodeViewDeleteCommand();

  // Remove deleted nodes
  foreach (Node *n, nodes_to_delete) {
    delete_command->AddNode(n, group_);
  }

  // Disconnect old edges
  foreach (const Node::OutputConnection &edge, copied_edges_) {
    bool found = false;
    const NodeInput &copy_input = edge.second;

    for (auto it=edge.first->output_connections().cbegin(); it!=edge.first->output_connections().cend(); it++) {
      if (it->second == copy_input) {
        found = true;
        break;
      }
    }

    if (!found) {
      // Edge has been disconnected
      delete_command->AddEdge(copy_subnodes_.key(edge.first), NodeInput(copy_subnodes_.key(copy_input.node()), copy_input.input(), copy_input.element()));
    }
  }

  delete_command->redo_now();

  // Add new nodes
  MultiUndoCommand *add_command = new MultiUndoCommand();
  for (auto it=copy_group_->GetContextPositions().cbegin(); it!=copy_group_->GetContextPositions().cend(); it++) {
    Node *n = it.key();
    Node *original = copy_subnodes_.key(n);
    if (!original) {
      // If it's a new node that isn't even in the project yet, add it
      original = n->copy();
      Node::CopyInputs(n, original, false);
      add_command->add_child(new NodeAddCommand(group_->parent(), original));
      copy_subnodes_.insert(original, n);
    }

    // Update position in context
    add_command->add_child(new NodeSetPositionCommand(original, group_, it.value()));
  }
  add_command->redo_now();

  // Connect new edges
  MultiUndoCommand *connect_command = new MultiUndoCommand();
  edges_to_connect = GetNewConnections(); // Update list with new nodes made above if necessary
  foreach (const Node::OutputConnection &edge, edges_to_connect) {
    connect_command->add_child(new NodeEdgeAddCommand(edge.first, edge.second));
  }
  connect_command->redo_now();

  MultiUndoCommand *command = new MultiUndoCommand();

  if (prepend_undo_) {
    command->add_child(prepend_undo_);
  }

  command->add_child(delete_command);
  command->add_child(add_command);
  command->add_child(connect_command);

  // Update group name
  if (name_edit_->text() != group_->GetCustomName()) {
    command->add_child(new NodeGroupSetCustomNameCommand(group_, name_edit_->text()));
  }

  Core::instance()->undo_stack()->push(command);

  super::accept();
}

void NodeGroupDialog::reject()
{
  if (prepend_undo_) {
    prepend_undo_->undo_now();
    delete prepend_undo_;
  }

  super::reject();
}

QVector<Node *> NodeGroupDialog::GetNodesToDelete()
{
  QVector<Node*> nodes_to_delete;

  for (auto it=group_->GetContextPositions().cbegin(); it!=group_->GetContextPositions().cend(); it++) {
    Node *original = it.key();
    Node *copy = copy_subnodes_.value(original);

    if (!copy_group_->ContextContainsNode(copy)) {
      nodes_to_delete.append(original);
    }
  }

  return nodes_to_delete;
}

QVector<Node::OutputConnection> NodeGroupDialog::GetNewConnections()
{
  QVector<Node::OutputConnection> edges_to_connect;

  for (auto it=copy_group_->GetContextPositions().cbegin(); it!=copy_group_->GetContextPositions().cend(); it++) {
    const Node::InputConnections &ic = it.key()->input_connections();

    for (auto jt=ic.cbegin(); jt!=ic.cend(); jt++) {
      // All connections inside this dialog will be valid and inside the group
      const NodeInput &copied_input = jt->first;
      Node *copied_output = jt->second;

      NodeInput original_input(copy_subnodes_.key(copied_input.node()), copied_input.input(), copied_input.element());
      Node *original_output = copy_subnodes_.key(copied_output);

      bool found = false;

      if (original_output) {
        for (auto kt=original_output->output_connections().cbegin(); kt!=original_output->output_connections().cend(); kt++) {
          if (kt->second == original_input) {
            found = true;
            break;
          }
        }
      }

      if (!found) {
        edges_to_connect.append({original_output, original_input});
      }
    }
  }

  return edges_to_connect;
}

bool NodeGroupDialog::OperationWillAffectOutsideGroup(const QVector<Node*> &deleted, const QVector<Node::OutputConnection> &connections)
{
  // Check if a node to be deleted inputs from a node outside the group
  foreach (Node *n, deleted) {
    for (auto jt=n->input_connections().cbegin(); jt!=n->input_connections().cend(); jt++) {
      if (!group_->ContextContainsNode(jt->second)) {
        return true;
      }
    }

    for (auto jt=n->output_connections().cbegin(); jt!=n->output_connections().cend(); jt++) {
      if (!group_->ContextContainsNode(jt->second.node())) {
        return true;
      }
    }
  }

  // Check if a new connection will overwrite a connection from outside the group
  foreach (const Node::OutputConnection &edge, connections) {
    if (Node *current_conn = edge.second.GetConnectedOutput()) {
      if (!group_->ContextContainsNode(current_conn)) {
        return true;
      }
    }
  }

  return false;
}

}

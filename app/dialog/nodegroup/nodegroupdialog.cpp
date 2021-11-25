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
#include <QSplitter>

#include "widget/nodeparamview/nodeparamview.h"
#include "widget/nodeview/nodeview.h"

namespace olive {

#define super QDialog

NodeGroupDialog::NodeGroupDialog(NodeGroup *group, QWidget *parent) :
  super(parent),
  group_(group),
  parent_undo_(nullptr)
{
  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Name:")), row, 0);

  name_edit_ = new QLineEdit();
  layout->addWidget(name_edit_, row, 1);

  row++;

  QSplitter *splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter, row, 0, 1, 2);

  NodeParamView *param_view = new NodeParamView(false);
  param_view->SetCreateCheckBoxes(kCheckBoxesOnNonConnected);
  splitter->addWidget(param_view);

  NodeView *node_view = new NodeView();
  node_view->SetContexts({group});
  QMetaObject::invokeMethod(node_view, &NodeView::CenterOnItemsBoundingRect, Qt::QueuedConnection);
  splitter->addWidget(node_view);

  for (auto it=group->GetInputPassthroughs().cbegin(); it!=group->GetInputPassthroughs().cend(); it++) {
    param_view->SetInputChecked(it.value(), true);
  }

  connect(node_view, &NodeView::NodesSelected, param_view, &NodeParamView::SelectNodes);
  connect(node_view, &NodeView::NodesDeselected, param_view, &NodeParamView::DeselectNodes);
  node_view->SelectAll();

  row++;

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btns->setCenterButtons(true);
  connect(btns, &QDialogButtonBox::accepted, this, &NodeGroupDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &NodeGroupDialog::reject);
  layout->addWidget(btns, row, 0, 1, 2);

  setWindowTitle(tr("Group Editor"));
}

void NodeGroupDialog::accept()
{
  MultiUndoCommand *command = new MultiUndoCommand();

  if (name_edit_->text() != group_->GetCustomName()) {
    command->add_child(new NodeGroupSetCustomNameCommand(group_, name_edit_->text()));
  }

  if (parent_undo_) {
    parent_undo_->add_child(command);
  } else {
    Core::instance()->undo_stack()->push(command);
  }

  super::accept();
}

}

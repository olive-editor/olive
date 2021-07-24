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

#include "nodepropertiesdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>

#include "core.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

NodePropertiesDialog::NodePropertiesDialog(Node *node, const rational &timebase, QWidget *parent) :
  QDialog(parent),
  node_(node)
{
  setWindowTitle(tr("Node Properties"));

  QVBoxLayout *layout = new QVBoxLayout(this);

  QHBoxLayout *label_layout = new QHBoxLayout();
  label_layout->setMargin(0);
  layout->addLayout(label_layout);

  label_layout->addWidget(new QLabel(tr("Name:")));

  label_edit_ = new QLineEdit();
  label_edit_->setText(node->GetLabel());
  label_layout->addWidget(label_edit_);

  NodeParamViewItem *item = new NodeParamViewItem(node);
  item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  item->SetTimebase(timebase);
  item->setTitleBarWidget(new QWidget());
  layout->addWidget(item);

  layout->addStretch();

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(btns, &QDialogButtonBox::accepted, this, &NodePropertiesDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &NodePropertiesDialog::reject);
  layout->addWidget(btns);
}

void NodePropertiesDialog::accept()
{
  if (label_edit_->text() != node_->GetLabel()) {
    NodeRenameCommand* rename_command = new NodeRenameCommand();
    rename_command->AddNode(node_, label_edit_->text());
    Core::instance()->undo_stack()->push(rename_command);
  }

  QDialog::accept();
}

}

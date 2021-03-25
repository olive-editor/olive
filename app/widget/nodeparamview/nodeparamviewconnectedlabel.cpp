/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "nodeparamviewconnectedlabel.h"

#include <QHBoxLayout>

#include "common/qtutils.h"
#include "core.h"
#include "node/node.h"
#include "widget/menu/menu.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

NodeParamViewConnectedLabel::NodeParamViewConnectedLabel(const NodeInput &input, QWidget *parent) :
  QWidget(parent),
  input_(input),
  connected_node_(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(QtUtils::QFontMetricsWidth(fontMetrics(), QStringLiteral(" ")));
  layout->setMargin(0);

  layout->addWidget(new QLabel(tr("Connected to")));

  connected_to_lbl_ = new ClickableLabel();
  connected_to_lbl_->setCursor(Qt::PointingHandCursor);
  connected_to_lbl_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(connected_to_lbl_, &ClickableLabel::MouseClicked, this, &NodeParamViewConnectedLabel::ConnectionClicked);
  connect(connected_to_lbl_, &ClickableLabel::customContextMenuRequested, this, &NodeParamViewConnectedLabel::ShowLabelContextMenu);
  layout->addWidget(connected_to_lbl_);

  layout->addStretch();

  // Set up "link" font
  QFont link_font = connected_to_lbl_->font();
  link_font.setUnderline(true);
  connected_to_lbl_->setForegroundRole(QPalette::Link);
  connected_to_lbl_->setFont(link_font);

  if (input_.IsConnected()) {
    InputConnected(input_.GetConnectedOutput(), input_);
  } else {
    InputDisconnected(NodeOutput(), input_);
  }

  connect(input_.node(), &Node::InputConnected, this, &NodeParamViewConnectedLabel::InputConnected);
  connect(input_.node(), &Node::InputDisconnected, this, &NodeParamViewConnectedLabel::InputDisconnected);
}

void NodeParamViewConnectedLabel::InputConnected(const NodeOutput& output, const NodeInput& input)
{
  if (input_ != input) {
    return;
  }

  connected_node_ = output;

  UpdateLabel();
}

void NodeParamViewConnectedLabel::InputDisconnected(const NodeOutput &output, const NodeInput &input)
{
  if (input_ != input) {
    return;
  }

  Q_UNUSED(output)

  connected_node_ = NodeOutput();

  UpdateLabel();
}

void NodeParamViewConnectedLabel::ShowLabelContextMenu()
{
  Menu m(this);

  QAction* disconnect_action = m.addAction(tr("Disconnect"));
  connect(disconnect_action, &QAction::triggered, this, [this](){
    Core::instance()->undo_stack()->push(new NodeEdgeRemoveCommand(connected_node_, input_));
  });

  m.exec(QCursor::pos());
}

void NodeParamViewConnectedLabel::ConnectionClicked()
{
  if (connected_node_.IsValid()) {
    emit RequestSelectNode({connected_node_.node()});
  }
}

void NodeParamViewConnectedLabel::UpdateLabel()
{
  QString s;

  if (connected_node_.IsValid()) {
    s = connected_node_.node()->Name();
  } else {
    s = tr("Nothing");
  }

  connected_to_lbl_->setText(s);
}

}

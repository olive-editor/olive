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

#include "nodeparamviewitem.h"

#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <QMessageBox>
#include <QPainter>

#include "nodeparamviewkeyframecontrol.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"
#include "undo/undostack.h"

NodeParamViewItem::NodeParamViewItem(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setSpacing(0);
  main_layout->setMargin(0);

  // Create title bar widget
  title_bar_ = new NodeParamViewItemTitleBar(this);
  title_bar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  QHBoxLayout* title_bar_layout = new QHBoxLayout(title_bar_);

  title_bar_collapse_btn_ = new QPushButton();
  title_bar_collapse_btn_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  title_bar_collapse_btn_->setStyleSheet("border: none; background: none;");
  title_bar_collapse_btn_->setCheckable(true);
  title_bar_collapse_btn_->setChecked(true);

  // FIXME: Revise icon sizing algorithm (share with NodeViewItem)
  title_bar_collapse_btn_->setIconSize(QSize(fontMetrics().height()/2, fontMetrics().height()/2));

  connect(title_bar_collapse_btn_, SIGNAL(clicked(bool)), this, SLOT(SetExpanded(bool)));
  title_bar_layout->addWidget(title_bar_collapse_btn_);

  title_bar_lbl_ = new QLabel(title_bar_);
  title_bar_layout->addWidget(title_bar_lbl_);

  // Add title bar to widget
  main_layout->addWidget(title_bar_);

  // Create and add contents widget
  contents_ = new QWidget(this);
  content_layout_ = new QGridLayout(contents_);
  main_layout->addWidget(contents_);

  // Set correct widget state
  SetExpanded(title_bar_collapse_btn_->isChecked());
}

void NodeParamViewItem::AttachNode(Node *n)
{
  // Make sure we can attach this node (CanAddNode() should be run by the caller to make sure this node is valid)
  Q_ASSERT(CanAddNode(n));

  // Add node to the list
  nodes_.append(n);

  // If the added node was the first node, set up the UI
  if (nodes_.size() == 1) {
    SetupUI();
  } else {
    AddAdditionalNode(n);
  }
}

bool NodeParamViewItem::CanAddNode(Node* n)
{
  // Ensures that all Nodes have the same ID
  return (nodes_.isEmpty() || nodes_.first()->id() == n->id());
}

void NodeParamViewItem::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange && !nodes_.isEmpty()) {
    Retranslate();
  }

  QWidget::changeEvent(e);
}

void NodeParamViewItem::SetupUI()
{
  Q_ASSERT(!nodes_.isEmpty());

  Node* first_node = nodes_.first();

  int row_count = 0;

  foreach (NodeParam* param, first_node->parameters()) {
    // This widget only needs to show input parameters
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      // Add descriptor label
      QLabel* param_label = new QLabel();
      param_lbls_.append(param_label);

      content_layout_->addWidget(param_label, row_count, 0);

      // Create a widget/input bridge for this input
      NodeParamViewWidgetBridge* bridge = new NodeParamViewWidgetBridge(this);
      bridge->AddInput(input);
      bridges_.append(bridge);

      // Add widgets for this parameter ot the layout
      const QList<QWidget*>& widgets_for_param = bridge->widgets();
      for (int i=0;i<widgets_for_param.size();i++) {
        content_layout_->addWidget(widgets_for_param.at(i), row_count, i + 1);
      }

      // Add keyframe control to this layout if parameter is keyframable
      if (input->is_keyframable()) {
        // Hacky but effective way to make sure this widget is always as far right as possible
        int control_column = 10;

        NodeParamViewKeyframeControl* key_control = new NodeParamViewKeyframeControl(input);
        content_layout_->addWidget(key_control, row_count, control_column);
      }

      row_count++;
    }
  }

  Retranslate();
}

void NodeParamViewItem::AddAdditionalNode(Node *n)
{
  int bridge_count = 0;

  foreach (NodeParam* param, n->parameters()) {
    if (param->type() == NodeParam::kInput) {
      bridges_.at(bridge_count)->AddInput(static_cast<NodeInput*>(param));

      bridge_count++;
    }
  }
}

void NodeParamViewItem::Retranslate()
{
  Node* first_node = nodes_.first();

  first_node->Retranslate();

  title_bar_lbl_->setText(first_node->Name());

  int row_count = 0;

  foreach (NodeParam* param, first_node->parameters()) {
    // This widget only needs to show input parameters
    if (param->type() == NodeParam::kInput) {
      param_lbls_.at(row_count)->setText(tr("%1:").arg(param->name()));

      row_count++;
    }
  }
}

void NodeParamViewItem::SetExpanded(bool e)
{
  expanded_ = e;

  contents_->setVisible(e);

  if (expanded_) {
    title_bar_collapse_btn_->setIcon(olive::icon::TriDown);
  } else {
    title_bar_collapse_btn_->setIcon(olive::icon::TriRight);
  }
}

void NodeParamViewItem::KeyframeEnableChanged(bool e)
{
  NodeParamViewKeyframeControl* control = static_cast<NodeParamViewKeyframeControl*>(sender());
  NodeInput* input = control->GetConnectedInput();

  if (e == input->is_keyframing()) {
    // No-op
    return;
  }

  if (e) {
    QUndoCommand* command = new QUndoCommand();

    // Enable keyframing
    new NodeParamSetKeyframing(input, true, command);

    // FIXME: Create a keyframe at this time

    olive::undo_stack.push(command);
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                         tr("Warning"),
                         tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Disable keyframing
      QUndoCommand* command = new QUndoCommand();

      // FIXME: Delete all keyframes

      // Disable keyframing
      new NodeParamSetKeyframing(input, false, command);

      olive::undo_stack.push(command);

    } else {
      // Disable action has effectively been ignored
      control->SetKeyframeEnabled(true);
    }
  }
}

NodeParamViewItemTitleBar::NodeParamViewItemTitleBar(QWidget *parent) :
  QWidget(parent)
{
}

void NodeParamViewItemTitleBar::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  QPainter p(this);

  // Draw bottom border using text color
  int bottom = height() - 1;
  p.setPen(palette().text().color());
  p.drawLine(0, bottom, width(), bottom);
}

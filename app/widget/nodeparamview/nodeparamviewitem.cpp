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

#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"
#include "undo/undostack.h"

NodeParamViewItem::NodeParamViewItem(Node *node, QWidget *parent) :
  QWidget(parent),
  node_(node)
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

  SetupUI();
}

void NodeParamViewItem::SetTime(const rational &time)
{
  time_ = time;

  foreach (NodeParamViewWidgetBridge* bridge, bridges_) {
    // Updates the UI widgets from the time
    bridge->SetTime(time_);
  }

  foreach (NodeParamViewKeyframeControl* key_control, key_control_list_) {
    UpdateKeyframeControl(key_control);
  }
}

void NodeParamViewItem::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  QWidget::changeEvent(e);
}

void NodeParamViewItem::InputAddedKeyframeInternal(NodeInput *input, NodeKeyframePtr keyframe)
{
  // Find its row in the parameters
  QLabel* lbl = label_map_.value(input);

  // Find label's Y position
  QPoint lbl_center = lbl->rect().center();

  // Find global position
  lbl_center = lbl->mapToGlobal(lbl_center);

  emit KeyframeAdded(keyframe, lbl_center.y());
}

void NodeParamViewItem::SetupUI()
{
  int row_count = 0;

  foreach (NodeParam* param, node_->parameters()) {
    // This widget only needs to show input parameters
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      // Add descriptor label
      QLabel* param_label = new QLabel();
      param_lbls_.append(param_label);

      label_map_.insert(input, param_label);

      content_layout_->addWidget(param_label, row_count, 0);

      // Create a widget/input bridge for this input
      NodeParamViewWidgetBridge* bridge = new NodeParamViewWidgetBridge(input, this);
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
        connect(key_control, &NodeParamViewKeyframeControl::KeyframeEnableChanged, this, &NodeParamViewItem::UserChangedKeyframeEnable);
        connect(key_control, &NodeParamViewKeyframeControl::GoToPreviousKey, this, &NodeParamViewItem::GoToPreviousKey);
        connect(key_control, &NodeParamViewKeyframeControl::GoToNextKey, this, &NodeParamViewItem::GoToNextKey);
        connect(key_control, &NodeParamViewKeyframeControl::KeyframeToggled, this, &NodeParamViewItem::UserToggledKeyframe);
        key_control_list_.append(key_control);

        connect(input, &NodeInput::KeyframeEnableChanged, this, &NodeParamViewItem::InputKeyframeEnableChanged);
        connect(input, &NodeInput::KeyframeAdded, this, &NodeParamViewItem::InputAddedKeyframe);
        connect(input, &NodeInput::KeyframeRemoved, this, &NodeParamViewItem::KeyframeRemoved);
      }

      row_count++;
    }
  }

  Retranslate();
}

void NodeParamViewItem::Retranslate()
{
  node_->Retranslate();

  title_bar_lbl_->setText(node_->Name());

  int row_count = 0;

  foreach (NodeParam* param, node_->parameters()) {
    // This widget only needs to show input parameters
    if (param->type() == NodeParam::kInput) {
      param_lbls_.at(row_count)->setText(tr("%1:").arg(param->name()));

      row_count++;
    }
  }
}

void NodeParamViewItem::UpdateKeyframeControl(NodeParamViewKeyframeControl *key_control)
{
  NodeInput* input = key_control->GetConnectedInput();

  // Update UI based on time
  key_control->SetPreviousButtonEnabled(time_ > input->keyframes().first()->time());
  key_control->SetNextButtonEnabled(time_ < input->keyframes().last()->time());
  key_control->SetToggleButtonChecked(input->has_keyframe_at_time(time_));
}

NodeParamViewKeyframeControl *NodeParamViewItem::KeyframeControlFromInput(NodeInput *input) const
{
  foreach (NodeParamViewKeyframeControl* key_control, key_control_list_) {
    if (key_control->GetConnectedInput() == input) {
      return key_control;
    }
  }

  return nullptr;
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

void NodeParamViewItem::UserChangedKeyframeEnable(bool e)
{
  NodeParamViewKeyframeControl* control = static_cast<NodeParamViewKeyframeControl*>(sender());
  NodeInput* input = control->GetConnectedInput();

  if (e == input->is_keyframing()) {
    // No-op
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (e) {
    // Enable keyframing
    new NodeParamSetKeyframingCommand(input, true, command);

    // NodeInputs already have one keyframe by default, we move it to the current time here
    new NodeParamSetKeyframeTimeCommand(input->keyframes().first(), time_, command);
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                         tr("Warning"),
                         tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Store value at this time, we'll set this as the persistent value later
      QVariant stored_val = input->get_value_at_time(time_);

      // Delete all keyframes EXCEPT ONE
      for (int i=input->keyframes().size()-1;i>0;i--) {
        new NodeParamRemoveKeyframeCommand(input, input->keyframes().at(i), command);
      }

      // Update value with this one
      new NodeParamSetKeyframeValueCommand(input->keyframes().first(), stored_val, command);

      // Disable keyframing
      new NodeParamSetKeyframingCommand(input, false, command);

    } else {
      // Disable action has effectively been ignored
      control->SetKeyframeEnabled(true);
    }
  }

  olive::undo_stack.pushIfHasChildren(command);
}

void NodeParamViewItem::UserToggledKeyframe(bool e)
{
  NodeParamViewKeyframeControl* control = static_cast<NodeParamViewKeyframeControl*>(sender());
  NodeInput* input = control->GetConnectedInput();
  NodeKeyframePtr key = input->get_keyframe_at_time(time_);

  QUndoCommand* command = new QUndoCommand();

  if (e && !key) {
    // Add a keyframe here
    NodeKeyframePtr closest_key = input->get_closest_keyframe_to_time(time_);

    key = std::make_shared<NodeKeyframe>(time_, input->get_value_at_time(time_), closest_key->type());

    new NodeParamInsertKeyframeCommand(input, key, command);
  } else if (!e && key) {
    // Remove a keyframe here
    new NodeParamRemoveKeyframeCommand(input, key, command);
  }

  olive::undo_stack.pushIfHasChildren(command);
}

void NodeParamViewItem::InputKeyframeEnableChanged(bool e)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  foreach (NodeKeyframePtr key, input->keyframes()) {
    if (e) {
      // Add a keyframe item for each keyframe
      InputAddedKeyframeInternal(input, key);
    } else {
      // Remove each keyframe item
      emit KeyframeRemoved(key);
    }
  }
}

void NodeParamViewItem::InputAddedKeyframe(NodeKeyframePtr key)
{
  // Get NodeInput that emitted this signal
  NodeInput* input = static_cast<NodeInput*>(sender());

  InputAddedKeyframeInternal(input, key);

  UpdateKeyframeControl(KeyframeControlFromInput(input));
}

void NodeParamViewItem::GoToPreviousKey()
{
  NodeParamViewKeyframeControl* key_control = static_cast<NodeParamViewKeyframeControl*>(sender());
  NodeInput* input = key_control->GetConnectedInput();

  for (int i=input->keyframes().size()-1;i>=0;i--) {
    // Find closest keyframe that is before this time
    const rational& this_key_time = input->keyframes().at(i)->time();

    if (this_key_time < time_) {
      emit RequestSetTime(this_key_time);
      break;
    }
  }
}

void NodeParamViewItem::GoToNextKey()
{
  NodeParamViewKeyframeControl* key_control = static_cast<NodeParamViewKeyframeControl*>(sender());
  NodeInput* input = key_control->GetConnectedInput();

  for (int i=0;i<input->keyframes().size();i++) {
    // Find closest keyframe that is before this time
    const rational& this_key_time = input->keyframes().at(i)->time();

    if (this_key_time > time_) {
      emit RequestSetTime(this_key_time);
      break;
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

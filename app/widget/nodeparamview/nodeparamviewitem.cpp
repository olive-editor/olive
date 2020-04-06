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
#include <QPainter>

#include "core.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

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

  connect(title_bar_collapse_btn_, &QPushButton::clicked, this, &NodeParamViewItem::SetExpanded);
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

void NodeParamViewItem::SetTimeTarget(Node *target)
{
  foreach (NodeParamViewKeyframeControl* control, key_control_list_) {
    control->SetTimeTarget(target);
  }

  foreach (NodeParamViewWidgetBridge* bridge, bridges_) {
    bridge->SetTimeTarget(target);
  }
}

void NodeParamViewItem::SetTime(const rational &time)
{
  time_ = time;

  foreach (NodeParamViewWidgetBridge* bridge, bridges_) {
    // Updates the UI widgets from the time
    bridge->SetTime(time_);
  }

  foreach (NodeParamViewKeyframeControl* key_control, key_control_list_) {
    key_control->SetTime(time_);
  }
}

void NodeParamViewItem::SignalAllKeyframes()
{
  foreach (NodeParam* param, node_->parameters()) {
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      foreach (const NodeInput::KeyframeTrack& track, input->keyframe_tracks()) {
        foreach (NodeKeyframePtr key, track) {
          InputAddedKeyframeInternal(input, key);
        }
      }
    }
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
      ClickableLabel* param_label = new ClickableLabel();
      connect(param_label, &ClickableLabel::MouseClicked, this, &NodeParamViewItem::LabelClicked);
      param_lbls_.append(param_label);

      label_map_.insert(input, param_label);

      content_layout_->addWidget(param_label, row_count, 0);

      // Create a widget/input bridge for this input
      NodeParamViewWidgetBridge* bridge = new NodeParamViewWidgetBridge(input, this);
      bridges_.append(bridge);

      // Add widgets for this parameter to the layout
      const QList<QWidget*>& widgets_for_param = bridge->widgets();
      for (int i=0;i<widgets_for_param.size();i++) {
        content_layout_->addWidget(widgets_for_param.at(i), row_count, i + 1);
      }

      // Add keyframe control to this layout if parameter is keyframable
      if (input->is_keyframable()) {
        // Hacky but effective way to make sure this widget is always as far right as possible
        int control_column = 10;

        NodeParamViewKeyframeControl* key_control = new NodeParamViewKeyframeControl();
        key_control->SetInput(input);
        content_layout_->addWidget(key_control, row_count, control_column);
        connect(key_control, &NodeParamViewKeyframeControl::RequestSetTime, this, &NodeParamViewItem::RequestSetTime);
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
    title_bar_collapse_btn_->setIcon(icon::TriDown);
  } else {
    title_bar_collapse_btn_->setIcon(icon::TriRight);
  }
}

void NodeParamViewItem::InputKeyframeEnableChanged(bool e)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  foreach (const NodeInput::KeyframeTrack& track, input->keyframe_tracks()) {
    foreach (NodeKeyframePtr key, track) {
      if (e) {
        // Add a keyframe item for each keyframe
        InputAddedKeyframeInternal(input, key);
      } else {
        // Remove each keyframe item
        emit KeyframeRemoved(key);
      }
    }
  }
}

void NodeParamViewItem::InputAddedKeyframe(NodeKeyframePtr key)
{
  // Get NodeInput that emitted this signal
  NodeInput* input = static_cast<NodeInput*>(sender());

  InputAddedKeyframeInternal(input, key);
}

void NodeParamViewItem::LabelClicked()
{
  ClickableLabel* lbl = static_cast<ClickableLabel*>(sender());

  NodeInput* input = label_map_.key(lbl, nullptr);

  if (input) {
    emit InputClicked(input);
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

OLIVE_NAMESPACE_EXIT

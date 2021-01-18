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

#include "nodeparamviewitem.h"

#include <QCheckBox>
#include <QDebug>
#include <QEvent>
#include <QPainter>

#include "core.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"

namespace olive {

NodeParamViewItem::NodeParamViewItem(Node *node, QWidget *parent) :
  QDockWidget(parent),
  node_(node),
  highlighted_(false)
{
  // Create title bar widget
  title_bar_ = new NodeParamViewItemTitleBar(this);

  // Add title bar to widget
  this->setTitleBarWidget(title_bar_);

  // Create and add contents widget
  body_ = new NodeParamViewItemBody(node_);
  connect(body_, &NodeParamViewItemBody::RequestSelectNode, this, &NodeParamViewItem::RequestSelectNode);
  connect(body_, &NodeParamViewItemBody::RequestSetTime, this, &NodeParamViewItem::RequestSetTime);
  connect(body_, &NodeParamViewItemBody::KeyframeAdded, this, &NodeParamViewItem::KeyframeAdded);
  connect(body_, &NodeParamViewItemBody::KeyframeRemoved, this, &NodeParamViewItem::KeyframeRemoved);
  connect(title_bar_, &NodeParamViewItemTitleBar::ExpandedStateChanged, this, &NodeParamViewItem::SetExpanded);
  connect(title_bar_, &NodeParamViewItemTitleBar::PinToggled, this, &NodeParamViewItem::PinToggled);

  this->setWidget(body_);

  connect(node_, &Node::LabelChanged, this, &NodeParamViewItem::Retranslate);

  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

  setFocusPolicy(Qt::ClickFocus);

  Retranslate();
}

void NodeParamViewItem::SetTimeTarget(Node *target)
{
  body_->SetTimeTarget(target);
}

void NodeParamViewItem::SetTime(const rational &time)
{
  time_ = time;

  body_->SetTime(time_);
}

Node *NodeParamViewItem::GetNode() const
{
  return node_;
}

void NodeParamViewItem::SignalAllKeyframes()
{
  body_->SignalAllKeyframes();
}

void NodeParamViewItem::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  QWidget::changeEvent(e);
}

void NodeParamViewItem::paintEvent(QPaintEvent *event)
{
  QDockWidget::paintEvent(event);

  // Draw border if focused
  if (highlighted_) {
    QPainter p(this);
    p.setBrush(Qt::NoBrush);
    p.setPen(palette().highlight().color());
    p.drawRect(rect().adjusted(0, 0, -1, -1));
  }
}

void NodeParamViewItem::Retranslate()
{
  node_->Retranslate();

  if (node_->GetLabel().isEmpty()) {
    title_bar_->SetText(node_->Name());
  } else {
    title_bar_->SetText(tr("%1 (%2)").arg(node_->GetLabel(), node_->Name()));
  }

  body_->Retranslate();
}

void NodeParamViewItem::SetExpanded(bool e)
{
  body_->setVisible(e);
  title_bar_->SetExpanded(e);
}

bool NodeParamViewItem::IsExpanded() const
{
  return body_->isVisible();
}

void NodeParamViewItem::ToggleExpanded()
{
  SetExpanded(!IsExpanded());
}

NodeParamViewItemTitleBar::NodeParamViewItemTitleBar(QWidget *parent) :
  QWidget(parent),
  draw_border_(true)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  collapse_btn_ = new CollapseButton();
  connect(collapse_btn_, &QPushButton::clicked, this, &NodeParamViewItemTitleBar::ExpandedStateChanged);
  layout->addWidget(collapse_btn_);

  lbl_ = new QLabel();
  layout->addWidget(lbl_);

  // Place next buttons on the far side
  layout->addStretch();

  QPushButton* pin_btn = new QPushButton(QStringLiteral("P"));
  pin_btn->setCheckable(true);
  pin_btn->setFixedSize(pin_btn->sizeHint().height(), pin_btn->sizeHint().height());
  layout->addWidget(pin_btn);
  connect(pin_btn, &QPushButton::clicked, this, &NodeParamViewItemTitleBar::PinToggled);
}

void NodeParamViewItemTitleBar::SetExpanded(bool e)
{
  draw_border_ = e;
  collapse_btn_->setChecked(e);

  update();
}

void NodeParamViewItemTitleBar::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (draw_border_) {
    QPainter p(this);

    // Draw bottom border using text color
    int bottom = height() - 1;
    p.setPen(palette().text().color());
    p.drawLine(0, bottom, width(), bottom);
  }
}

void NodeParamViewItemTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
  QWidget::mouseDoubleClickEvent(event);

  collapse_btn_->click();
}

NodeParamViewItemBody::NodeParamViewItemBody(Node* node, QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* root_layout = new QGridLayout(this);

  // Create widgets all root level components
  for (int i=0; i<node->inputs().size(); i++) {
    NodeInput* input = node->inputs().at(i);

    CreateWidgets(root_layout, input, -1, i);
  }
}

void NodeParamViewItemBody::CreateWidgets(QGridLayout* layout, NodeInput *input, int element, int row)
{
  InputUI ui_objects;

  // Add descriptor label
  ui_objects.main_label = new QLabel();

  // Label always goes into column 1 (array collapse button goes into 0 if applicable)
  layout->addWidget(ui_objects.main_label, row, 1);

  if (input->IsArray() && element == -1) {
    // Create a collapse toggle for expanding/collapsing the array
    CollapseButton* array_collapse_btn = new CollapseButton();

    // Collapse button always goes into column 0
    layout->addWidget(array_collapse_btn, row, 0);


    /*
    QVector<NodeConnectable::InputConnection> subelements(conn.input->ArraySize());

    for (int i=0; i<conn.input->ArraySize(); i++) {
      subelements[i] = {conn.input, i};
    }

    NodeParamViewItemBody* sub_body = new NodeParamViewItemBody(subelements);
    sub_bodies_.append(sub_body);
    sub_body->layout()->setMargin(0);
    content_layout->addWidget(sub_body, row_count + 1, 0, 1, max_col + 1);

    connect(array_collapse_btn, &CollapseButton::toggled, sub_body, &NodeParamViewItemBody::setVisible);

    connect(sub_body, &NodeParamViewItemBody::KeyframeAdded, this, &NodeParamViewItemBody::KeyframeAdded);
    connect(sub_body, &NodeParamViewItemBody::KeyframeRemoved, this, &NodeParamViewItemBody::KeyframeRemoved);
    connect(sub_body, &NodeParamViewItemBody::RequestSetTime, this, &NodeParamViewItemBody::RequestSetTime);
    connect(sub_body, &NodeParamViewItemBody::RequestSelectNode, this, &NodeParamViewItemBody::RequestSelectNode);*/
  }

  // Create a widget/input bridge for this input
  ui_objects.widget_bridge = new NodeParamViewWidgetBridge(input, element, this);

  // 0 is for the array collapse button, 1 is for the main label, widgets start at 2
  const int widget_start = 2;

  // Add widgets for this parameter to the layout
  for (int i=0; i<ui_objects.widget_bridge->widgets().size(); i++) {
    QWidget* w = ui_objects.widget_bridge->widgets().at(i);

    layout->addWidget(w, row, i+widget_start);
  }

  if (input->IsConnectable()) {
    // Create clickable label used when an input is connected
    ui_objects.connected_label = new NodeParamViewConnectedLabel(input, element);
    connect(ui_objects.connected_label, &NodeParamViewConnectedLabel::ConnectionClicked, this, &NodeParamViewItemBody::ConnectionClicked);
    layout->addWidget(ui_objects.connected_label, row, widget_start);

    connect(input, &NodeInput::InputConnected, this, &NodeParamViewItemBody::EdgeChanged);
    connect(input, &NodeInput::InputDisconnected, this, &NodeParamViewItemBody::EdgeChanged);
  }

  // Add keyframe control to this layout if parameter is keyframable
  if (input->IsKeyframable()) {
    // We make an assumption here that there will never be more than 7 widgets and so the 10th
    // column will be free
    const int control_column = 10;

    ui_objects.key_control = new NodeParamViewKeyframeControl();
    ui_objects.key_control->SetInput(input, element);
    layout->addWidget(ui_objects.key_control, row, control_column);
    connect(ui_objects.key_control, &NodeParamViewKeyframeControl::RequestSetTime, this, &NodeParamViewItemBody::RequestSetTime);

    connect(input, &NodeInput::KeyframeEnableChanged, this, &NodeParamViewItemBody::InputKeyframeEnableChanged);
    connect(input, &NodeInput::KeyframeAdded, this, &NodeParamViewItemBody::InputAddedKeyframe);
    connect(input, &NodeInput::KeyframeRemoved, this, &NodeParamViewItemBody::KeyframeRemoved);
  }

  input_ui_map_.insert(Node::InputConnection(input, element), ui_objects);

  if (input->IsConnectable()) {
    UpdateUIForEdgeConnection(input, element);
  }
}

void NodeParamViewItemBody::SetTimeTarget(Node *target)
{
  foreach (const InputUI& ui_obj, input_ui_map_) {
    // Only keyframable inputs have a key control widget
    if (ui_obj.key_control) {
      ui_obj.key_control->SetTimeTarget(target);
    }

    ui_obj.widget_bridge->SetTimeTarget(target);
  }

  foreach (NodeParamViewItemBody* sb, sub_bodies_) {
    sb->SetTimeTarget(target);
  }
}

void NodeParamViewItemBody::SetTime(const rational &time)
{
  foreach (const InputUI& ui_obj, input_ui_map_) {
    // Only keyframable inputs have a key control widget
    if (ui_obj.key_control) {
      ui_obj.key_control->SetTime(time);
    }

    ui_obj.widget_bridge->SetTime(time);
  }

  foreach (NodeParamViewItemBody* sb, sub_bodies_) {
    sb->SetTime(time);
  }
}

void NodeParamViewItemBody::Retranslate()
{
  for (auto i=input_ui_map_.begin(); i!=input_ui_map_.end(); i++) {
    i.value().main_label->setText(tr("%1:").arg(i.key().input->name()));
  }

  foreach (NodeParamViewItemBody* sb, sub_bodies_) {
    sb->Retranslate();
  }
}

void NodeParamViewItemBody::SignalAllKeyframes()
{
  for (auto i=input_ui_map_.begin(); i!=input_ui_map_.end(); i++) {
    NodeInput* input = i.key().input;

    foreach (const NodeKeyframeTrack& track, input->GetKeyframeTracks(i.key().element)) {
      foreach (NodeKeyframe* key, track) {
        InputAddedKeyframeInternal(input, key);
      }
    }
  }

  foreach (NodeParamViewItemBody* sb, sub_bodies_) {
    sb->SignalAllKeyframes();
  }
}

void NodeParamViewItemBody::EdgeChanged(Node* src, int element)
{
  Q_UNUSED(src)

  UpdateUIForEdgeConnection(static_cast<NodeInput*>(sender()), element);
}

void NodeParamViewItemBody::UpdateUIForEdgeConnection(NodeInput *input, int element)
{
  // Show/hide bridge widgets
  const InputUI& ui_objects = input_ui_map_[{input, element}];

  foreach (QWidget* w, ui_objects.widget_bridge->widgets()) {
    w->setVisible(!input->IsConnected(element));
  }

  // Show/hide connection label
  ui_objects.connected_label->setVisible(input->IsConnected(element));
}

void NodeParamViewItemBody::InputKeyframeEnableChanged(bool e, int element)
{
  NodeInput* input = static_cast<NodeInput*>(sender());

  foreach (const NodeKeyframeTrack& track, input->GetKeyframeTracks(element)) {
    foreach (NodeKeyframe* key, track) {
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

void NodeParamViewItemBody::InputAddedKeyframe(NodeKeyframe* key)
{
  // Get NodeInput that emitted this signal
  NodeInput* input = static_cast<NodeInput*>(sender());

  InputAddedKeyframeInternal(input, key);
}

void NodeParamViewItemBody::ConnectionClicked()
{
  for (auto iterator=input_ui_map_.begin(); iterator!=input_ui_map_.end(); iterator++) {
    if (iterator.value().connected_label == sender()) {
      Node* connected = iterator.key().input->parent();

      if (connected) {
        emit RequestSelectNode({connected});
      }

      return;
    }
  }
}

void NodeParamViewItemBody::InputAddedKeyframeInternal(NodeInput *input, NodeKeyframe* keyframe)
{
  // Find its row in the parameters
  QLabel* lbl = input_ui_map_.value(input).main_label;

  // Find label's Y position
  QPoint lbl_center = lbl->rect().center();

  // Find global position
  lbl_center = lbl->mapToGlobal(lbl_center);

  emit KeyframeAdded(keyframe, lbl_center.y());
}

NodeParamViewItemBody::InputUI::InputUI() :
  main_label(nullptr),
  widget_bridge(nullptr),
  connected_label(nullptr),
  key_control(nullptr)
{
}

}

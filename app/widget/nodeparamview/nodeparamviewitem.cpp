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

#include "common/qtutils.h"
#include "core.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"

namespace olive {

const int NodeParamViewItemBody::kKeyControlColumn = 10;
const int NodeParamViewItemBody::kArrayInsertColumn = kKeyControlColumn-1;
const int NodeParamViewItemBody::kArrayRemoveColumn = kArrayInsertColumn-1;

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
  connect(body_, &NodeParamViewItemBody::ArrayExpandedChanged, this, &NodeParamViewItem::ArrayExpandedChanged);
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

  emit ExpandedChanged(e);
}

bool NodeParamViewItem::IsExpanded() const
{
  return body_->isVisible();
}

int NodeParamViewItem::GetElementY(const NodeInput &c) const
{
  if (IsExpanded()) {
    return body_->GetElementY(c);
  } else {
    // Not expanded, put keyframes at the titlebar Y
    return mapToGlobal(title_bar_->rect().center()).y();
  }
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

  int insert_row = 0;

  // Create widgets all root level components
  foreach (const QString& input, node->inputs()) {
    CreateWidgets(root_layout, node, input, -1, insert_row);

    insert_row++;

    if (node->InputIsArray(input)) {
      // Insert here
      QWidget* array_widget = new QWidget();

      QGridLayout* array_layout = new QGridLayout(array_widget);
      array_layout->setContentsMargins(QtUtils::QFontMetricsWidth(fontMetrics(), QStringLiteral("    ")), 0, 0, 0);

      root_layout->addWidget(array_widget, insert_row, 1, 1, 10);

      int arr_sz = node->InputArraySize(input);
      for (int j=0; j<arr_sz; j++) {
        CreateWidgets(array_layout, node, input, j, j);
      }

      // Add one last add button for appending to the array
      NodeParamViewArrayButton* append_btn = new NodeParamViewArrayButton(NodeParamViewArrayButton::kAdd);
      connect(append_btn, &NodeParamViewArrayButton::clicked, this, &NodeParamViewItemBody::ArrayAppendClicked);
      array_layout->addWidget(append_btn, arr_sz, kArrayInsertColumn);

      array_widget->setVisible(false);

      array_ui_.insert({node, input}, {array_widget, arr_sz, append_btn});

      insert_row++;
    }
  }

  connect(node, &Node::InputArraySizeChanged, this, &NodeParamViewItemBody::InputArraySizeChanged);
  connect(node, &Node::InputConnected, this, &NodeParamViewItemBody::EdgeChanged);
  connect(node, &Node::InputDisconnected, this, &NodeParamViewItemBody::EdgeChanged);
}

void NodeParamViewItemBody::CreateWidgets(QGridLayout* layout, Node *node, const QString &input, int element, int row)
{
  NodeInput input_ref(node, input, element);

  InputUI ui_objects;

  // Add descriptor label
  ui_objects.main_label = new QLabel();

  // Label always goes into column 1 (array collapse button goes into 0 if applicable)
  layout->addWidget(ui_objects.main_label, row, 1);

  if (node->InputIsArray(input)) {
    if (element == -1) {

      // Create a collapse toggle for expanding/collapsing the array
      CollapseButton* array_collapse_btn = new CollapseButton();

      // Default to collapsed
      array_collapse_btn->setChecked(false);

      // Collapse button always goes into column 0
      layout->addWidget(array_collapse_btn, row, 0);

      // Connect signal to show/hide array params when toggled
      connect(array_collapse_btn, &CollapseButton::toggled, this, &NodeParamViewItemBody::ArrayCollapseBtnPressed);

      array_collapse_buttons_.insert({node, input}, array_collapse_btn);

    } else {

      NodeParamViewArrayButton* insert_element_btn = new NodeParamViewArrayButton(NodeParamViewArrayButton::kAdd);
      NodeParamViewArrayButton* remove_element_btn = new NodeParamViewArrayButton(NodeParamViewArrayButton::kRemove);

      layout->addWidget(insert_element_btn, row, kArrayInsertColumn);
      layout->addWidget(remove_element_btn, row, kArrayRemoveColumn);

      ui_objects.array_insert_btn = insert_element_btn;
      ui_objects.array_remove_btn = remove_element_btn;

      connect(insert_element_btn, &NodeParamViewArrayButton::clicked, this, &NodeParamViewItemBody::ArrayInsertClicked);
      connect(remove_element_btn, &NodeParamViewArrayButton::clicked, this, &NodeParamViewItemBody::ArrayRemoveClicked);

    }
  }

  // Create a widget/input bridge for this input
  ui_objects.widget_bridge = new NodeParamViewWidgetBridge(NodeInput(node, input, element), this);
  connect(ui_objects.widget_bridge, &NodeParamViewWidgetBridge::ArrayWidgetDoubleClicked, this, &NodeParamViewItemBody::ToggleArrayExpanded);

  // 0 is for the array collapse button, 1 is for the main label, widgets start at 2
  const int widget_start = 2;

  // Add widgets for this parameter to the layout
  for (int i=0; i<ui_objects.widget_bridge->widgets().size(); i++) {
    QWidget* w = ui_objects.widget_bridge->widgets().at(i);

    layout->addWidget(w, row, i+widget_start);
  }

  if (node->IsInputConnectable(input)) {
    // Create clickable label used when an input is connected
    ui_objects.connected_label = new NodeParamViewConnectedLabel(input_ref);
    connect(ui_objects.connected_label, &NodeParamViewConnectedLabel::RequestSelectNode, this, &NodeParamViewItemBody::RequestSelectNode);
    layout->addWidget(ui_objects.connected_label, row, widget_start);
  }

  // Add keyframe control to this layout if parameter is keyframable
  if (node->IsInputKeyframable(input)) {
    ui_objects.key_control = new NodeParamViewKeyframeControl();
    ui_objects.key_control->SetInput(input_ref);
    layout->addWidget(ui_objects.key_control, row, kKeyControlColumn);
    connect(ui_objects.key_control, &NodeParamViewKeyframeControl::RequestSetTime, this, &NodeParamViewItemBody::RequestSetTime);
  }

  input_ui_map_.insert(input_ref, ui_objects);

  if (node->IsInputConnectable(input)) {
    UpdateUIForEdgeConnection(input_ref);
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
}

void NodeParamViewItemBody::Retranslate()
{
  for (auto i=input_ui_map_.begin(); i!=input_ui_map_.end(); i++) {
    const NodeInput& ic = i.key();

    if (ic.IsArray() && ic.element() >= 0) {
      // Make the label the array index
      i.value().main_label->setText(tr("%n:", nullptr, ic.element()));
    } else {
      // Set to the input's name
      i.value().main_label->setText(tr("%1:").arg(ic.name()));
    }
  }
}

int NodeParamViewItemBody::GetElementY(NodeInput c) const
{
  if (c.IsArray() && !array_ui_.value(c.input_pair()).widget->isVisible()) {
    // Array is collapsed, so we'll return the Y of its root
    c.set_element(-1);
  }

  // Find its row in the parameters
  QLabel* lbl = input_ui_map_.value(c).main_label;

  // Find label's Y position
  QPoint lbl_center = lbl->rect().center();

  // Find global position
  lbl_center = lbl->mapToGlobal(lbl_center);

  // Return Y
  return lbl_center.y();
}

void NodeParamViewItemBody::EdgeChanged(const NodeOutput& output, const NodeInput& input)
{
  Q_UNUSED(output)

  UpdateUIForEdgeConnection(input);
}

void NodeParamViewItemBody::UpdateUIForEdgeConnection(const NodeInput& input)
{
  // Show/hide bridge widgets
  if (input_ui_map_.contains(input)) {
    const InputUI& ui_objects = input_ui_map_[input];

    foreach (QWidget* w, ui_objects.widget_bridge->widgets()) {
      w->setVisible(!input.IsConnected());
    }

    // Show/hide connection label
    ui_objects.connected_label->setVisible(input.IsConnected());
  }
}

void NodeParamViewItemBody::ArrayCollapseBtnPressed(bool checked)
{
  const NodeInputPair& input = array_collapse_buttons_.key(static_cast<CollapseButton*>(sender()));

  array_ui_.value(input).widget->setVisible(checked);

  emit ArrayExpandedChanged(checked);
}

void NodeParamViewItemBody::InputArraySizeChanged(const QString& input, int size)
{
  Node* node = static_cast<Node*>(sender());

  ArrayUI& array_ui = array_ui_[{node, input}];

  if (size != array_ui.count) {
    QGridLayout* grid = static_cast<QGridLayout*>(array_ui.widget->layout());

    if (array_ui.count < size) {
      // Our UI count is smaller than the size, create more
      grid->addWidget(array_ui.append_btn, size, kArrayInsertColumn);

      for (int i=array_ui.count; i<size; i++) {
        CreateWidgets(grid, node, input, i, i);
      }
    } else {
      for (int i=array_ui.count-1; i>=size; i--) {
        // Our UI count is larger than the size, delete
        InputUI input_ui = input_ui_map_.take({node, input, i});
        delete input_ui.main_label;
        qDeleteAll(input_ui.widget_bridge->widgets());
        delete input_ui.widget_bridge;
        delete input_ui.connected_label;
        delete input_ui.key_control;
        delete input_ui.array_insert_btn;
        delete input_ui.array_remove_btn;
      }

      grid->addWidget(array_ui.append_btn, size, kArrayInsertColumn);
    }

    array_ui.count = size;
  }

  Retranslate();
}

void NodeParamViewItemBody::ArrayAppendClicked()
{
  for (auto it=array_ui_.cbegin(); it!=array_ui_.cend(); it++) {
    if (it.value().append_btn == sender()) {
      it.key().node->InputArrayAppend(it.key().input, true);
      break;
    }
  }
}

void NodeParamViewItemBody::ArrayInsertClicked()
{
  for (auto it=input_ui_map_.cbegin(); it!=input_ui_map_.cend(); it++) {
    if (it.value().array_insert_btn == sender()) {
      // Found our input and element
      const NodeInput& ic = it.key();
      ic.node()->InputArrayInsert(ic.input(), ic.element(), true);
      break;
    }
  }
}

void NodeParamViewItemBody::ArrayRemoveClicked()
{
  for (auto it=input_ui_map_.cbegin(); it!=input_ui_map_.cend(); it++) {
    if (it.value().array_remove_btn == sender()) {
      // Found our input and element
      const NodeInput& ic = it.key();
      ic.node()->InputArrayRemove(ic.input(), ic.element(), true);
      break;
    }
  }
}

void NodeParamViewItemBody::ToggleArrayExpanded()
{
  NodeParamViewWidgetBridge* bridge = static_cast<NodeParamViewWidgetBridge*>(sender());

  for (auto it=input_ui_map_.cbegin(); it!=input_ui_map_.cend(); it++) {
    if (it.value().widget_bridge == bridge) {
      CollapseButton* b = array_collapse_buttons_.value(it.key().input_pair());
      b->setChecked(!b->isChecked());
      return;
    }
  }
}

NodeParamViewItemBody::InputUI::InputUI() :
  main_label(nullptr),
  widget_bridge(nullptr),
  connected_label(nullptr),
  key_control(nullptr),
  array_insert_btn(nullptr),
  array_remove_btn(nullptr)
{
}

}

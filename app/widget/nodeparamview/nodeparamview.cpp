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

#include "nodeparamview.h"

#include <QApplication>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>

#include "common/timecodefunctions.h"
#include "node/output/viewer/viewer.h"

namespace olive {

NodeParamView::NodeParamView(QWidget *parent) :
  TimeBasedWidget(true, false, parent),
  last_scroll_val_(0),
  focused_node_(nullptr)
{
  // Create horizontal layout to place scroll area in (and keyframe editing eventually)
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  // Set up scroll area for params
  QScrollArea* scroll_area = new QScrollArea();
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area->setWidgetResizable(true);
  splitter->addWidget(scroll_area);

  // Param widget
  param_widget_container_ = new NodeParamViewParamContainer();
  connect(param_widget_container_, &NodeParamViewParamContainer::Resized, this, &NodeParamView::UpdateGlobalScrollBar);
  scroll_area->setWidget(param_widget_container_);

  param_widget_area_ = new NodeParamViewDockArea();

  // Disable dock widgets from tabbing and disable glitchy animations
  param_widget_area_->setDockOptions(static_cast<QMainWindow::DockOption>(0));

  // HACK: Hide the main window separators (unfortunately the cursors still appear)
  param_widget_area_->setStyleSheet(QStringLiteral("QMainWindow::separator {background: rgba(0, 0, 0, 0)}"));

  QVBoxLayout* param_widget_container_layout = new QVBoxLayout(param_widget_container_);
  QMargins param_widget_margin = param_widget_container_layout->contentsMargins();
  param_widget_margin.setTop(ruler()->height());
  param_widget_container_layout->setContentsMargins(param_widget_margin);
  param_widget_container_layout->setSpacing(0);
  param_widget_container_layout->addWidget(param_widget_area_);

  param_widget_container_layout->addStretch(INT_MAX);

  // Set up keyframe view
  QWidget* keyframe_area = new QWidget();
  QVBoxLayout* keyframe_area_layout = new QVBoxLayout(keyframe_area);
  keyframe_area_layout->setSpacing(0);
  keyframe_area_layout->setMargin(0);

  // Create ruler object
  keyframe_area_layout->addWidget(ruler());

  // Create keyframe view
  keyframe_view_ = new KeyframeView();
  keyframe_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ConnectTimelineView(keyframe_view_);
  keyframe_area_layout->addWidget(keyframe_view_);

  // Connect ruler and keyframe view together
  connect(ruler(), &TimeRuler::TimeChanged, keyframe_view_, &KeyframeView::SetTime);
  connect(keyframe_view_, &KeyframeView::TimeChanged, ruler(), &TimeRuler::SetTime);
  connect(keyframe_view_, &KeyframeView::TimeChanged, this, &NodeParamView::SetTimestamp);
  connect(keyframe_view_, &KeyframeView::Dragged, this, &NodeParamView::KeyframeViewDragged);

  // Connect keyframe view scaling to this
  connect(keyframe_view_, &KeyframeView::ScaleChanged, this, &NodeParamView::SetScale);

  splitter->addWidget(keyframe_area);

  // Set both widgets to 50/50
  splitter->setSizes({INT_MAX, INT_MAX});

  // Disable collapsing param view (but collapsing keyframe view is permitted)
  splitter->setCollapsible(0, false);

  // Create global vertical scrollbar on the right
  vertical_scrollbar_ = new QScrollBar();
  vertical_scrollbar_->setMaximum(0);
  layout->addWidget(vertical_scrollbar_);

  // Connect scrollbars together
  connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
  connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, scroll_area->verticalScrollBar(), &QScrollBar::setValue);
  connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
  connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);
  connect(vertical_scrollbar_, &QScrollBar::valueChanged, scroll_area->verticalScrollBar(), &QScrollBar::setValue);
  connect(vertical_scrollbar_, &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);

  // TimeBasedWidget's scrollbar has extra functionality that we can take advantage of
  keyframe_view_->setHorizontalScrollBar(scrollbar());
  keyframe_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

  connect(keyframe_view_->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);

  // Set a default scale - FIXME: Hardcoded
  SetScale(120);

  SetMaximumScale(TimeBasedView::kMaximumScale);

  // Pickup on widget focus changes
  connect(qApp,
          &QApplication::focusChanged,
          this,
          &NodeParamView::FocusChanged);
}

void NodeParamView::SelectNodes(const QVector<Node *> &nodes)
{
  int original_node_count = items_.size();

  foreach (Node* n, nodes) {
    // If we've already added this node (either a duplicate or a pinned node), don't add another
    if (items_.contains(n)) {
      continue;
    }

    // Add to "active" list to represent currently selected node
    active_nodes_.append(n);

    // Create node UI
    AddNode(n);
  }

  if (items_.size() > original_node_count ) {
    UpdateItemTime(GetTimestamp());

    // Re-arrange keyframes
    QueueKeyframePositionUpdate();

    SignalNodeOrder();
  }
}

void NodeParamView::DeselectNodes(const QVector<Node *> &nodes)
{
  // Remove item from map and delete the widget
  int original_node_count = items_.size();

  foreach (Node* n, nodes) {
    // Filter out duplicates
    if (!items_.contains(n)) {
      continue;
    }

    if (!pinned_nodes_.contains(n)) {
      // Store expanded state
      node_expanded_state_.insert(n, items_.value(n)->IsExpanded());

      // Remove all keyframes from this node
      RemoveNode(n);
    }

    active_nodes_.removeOne(n);
  }

  if (items_.size() < original_node_count) {
    // Re-arrange keyframes
    QueueKeyframePositionUpdate();

    SignalNodeOrder();
  }
}

void NodeParamView::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  vertical_scrollbar_->setPageStep(vertical_scrollbar_->height());

  UpdateGlobalScrollBar();
}

void NodeParamView::ScaleChangedEvent(const double &scale)
{
  TimeBasedWidget::ScaleChangedEvent(scale);

  keyframe_view_->SetScale(scale);
}

void NodeParamView::TimebaseChangedEvent(const rational &timebase)
{
  TimeBasedWidget::TimebaseChangedEvent(timebase);

  keyframe_view_->SetTimebase(timebase);

  foreach (NodeParamViewItem* item, items_) {
      item->SetTimebase(timebase);
  }

  UpdateItemTime(GetTimestamp());
}

void NodeParamView::TimeChangedEvent(const int64_t &timestamp)
{
  TimeBasedWidget::TimeChangedEvent(timestamp);

  keyframe_view_->SetTime(timestamp);

  UpdateItemTime(timestamp);
}

void NodeParamView::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  // Set viewer as a time target
  keyframe_view_->SetTimeTarget(n);

  foreach (NodeParamViewItem* item, items_) {
    item->SetTimeTarget(n);
  }
}

Node *NodeParamView::GetTimeTarget() const
{
  return keyframe_view_->GetTimeTarget();
}

void NodeParamView::DeleteSelected()
{
  keyframe_view_->DeleteSelected();
}

void NodeParamView::UpdateItemTime(const int64_t &timestamp)
{
  rational time = Timecode::timestamp_to_time(timestamp, timebase());

  foreach (NodeParamViewItem* item, items_) {
    item->SetTime(time);
  }
}

void NodeParamView::QueueKeyframePositionUpdate()
{
  QMetaObject::invokeMethod(this, &NodeParamView::UpdateElementY, Qt::QueuedConnection);
}

void NodeParamView::SignalNodeOrder()
{
  // Sort by item Y (apparently there's no way in Qt to get the order of dock widgets)
  QVector<Node*> nodes;
  QVector<int> item_ys;

  for (auto it=items_.cbegin(); it!=items_.cend(); it++) {
    int item_y = it.value()->pos().y();

    bool inserted = false;

    for (int i=0; i<item_ys.size(); i++) {
      if (item_ys.at(i) > item_y) {
        item_ys.insert(i, item_y);
        nodes.insert(i, it.key());
        inserted = true;
        break;
      }
    }

    if (!inserted) {
      item_ys.append(item_y);
      nodes.append(it.key());
    }
  }

  emit NodeOrderChanged(nodes);
}

void NodeParamView::AddNode(Node *n)
{
  NodeParamViewItem* item = new NodeParamViewItem(n, param_widget_area_);

  item->setAllowedAreas(Qt::LeftDockWidgetArea);
  item->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
  item->SetExpanded(node_expanded_state_.value(n, true));

  connect(n, &Node::KeyframeAdded, keyframe_view_, &KeyframeView::AddKeyframe);
  connect(n, &Node::KeyframeRemoved, keyframe_view_, &KeyframeView::RemoveKeyframe);

  connect(item, &NodeParamViewItem::RequestSetTime, this, &NodeParamView::ItemRequestedTimeChanged);
  connect(item, &NodeParamViewItem::RequestSelectNode, this, &NodeParamView::RequestSelectNode);
  connect(item, &NodeParamViewItem::dockLocationChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
  connect(item, &NodeParamViewItem::dockLocationChanged, this, &NodeParamView::SignalNodeOrder);
  connect(item, &NodeParamViewItem::PinToggled, this, &NodeParamView::PinNode);
  connect(item, &NodeParamViewItem::ArrayExpandedChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
  connect(item, &NodeParamViewItem::ExpandedChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
  connect(item, &NodeParamViewItem::Moved, this, &NodeParamView::QueueKeyframePositionUpdate);

  // Set time target
  item->SetTimeTarget(GetTimeTarget());

  // Set the timebase
  item->SetTimebase(timebase());

  items_.insert(n, item);
  param_widget_area_->addDockWidget(Qt::LeftDockWidgetArea, item);

  if (!focused_node_ && n->HasGizmos()) {
    // We'll focus this node now
    item->SetHighlighted(true);
    focused_node_ = n;
    emit FocusedNodeChanged(focused_node_);
  }

  keyframe_view_->AddKeyframesOfNode(n);
}

void NodeParamView::RemoveNode(Node *n)
{
  keyframe_view_->RemoveKeyframesOfNode(n);

  disconnect(n, &Node::KeyframeAdded, keyframe_view_, &KeyframeView::AddKeyframe);
  disconnect(n, &Node::KeyframeRemoved, keyframe_view_, &KeyframeView::RemoveKeyframe);

  delete items_.take(n);

  if (focused_node_ == n) {
    // Try to find new node with gizmos to focus
    focused_node_ = nullptr;
    for (auto it=items_.cbegin(); it!=items_.cend(); it++) {
      if (it.key()->HasGizmos()) {
        focused_node_ = it.key();
        it.value()->SetHighlighted(true);
        break;
      }
    }

    emit FocusedNodeChanged(focused_node_);
  }
}

void NodeParamView::ItemRequestedTimeChanged(const rational &time)
{
  SetTimeAndSignal(Timecode::time_to_timestamp(time, keyframe_view_->timebase()));
}

void NodeParamView::UpdateGlobalScrollBar()
{
  int height_offscreen = param_widget_container_->height() - ruler()->height() + scrollbar()->height();

  keyframe_view_->SetMaxScroll(height_offscreen);
  vertical_scrollbar_->setRange(0, height_offscreen - keyframe_view_->height());
}

void NodeParamView::PinNode(bool pin)
{
  NodeParamViewItem* item = static_cast<NodeParamViewItem*>(sender());
  Node* node = item->GetNode();

  if (pin) {
    pinned_nodes_.append(node);
  } else {
    pinned_nodes_.removeOne(node);

    if (!active_nodes_.contains(node)) {
      RemoveNode(node);
      SignalNodeOrder();
    }
  }
}

void NodeParamView::FocusChanged(QWidget* old, QWidget* now)
{
  Q_UNUSED(old)

  QObject* parent = now;
  NodeParamViewItem* item;

  while (parent) {
    item = dynamic_cast<NodeParamViewItem*>(parent);

    if (item) {
      // Found it!
      if (item->GetNode() != focused_node_) {
        if (focused_node_) {
          // De-focus current node
          items_.value(focused_node_)->SetHighlighted(false);
        }

        focused_node_ = item->GetNode();

        item->SetHighlighted(true);

        emit FocusedNodeChanged(focused_node_);
      }

      break;
    }

    parent = parent->parent();
  }
}

void NodeParamView::KeyframeViewDragged(int x, int y)
{
  Q_UNUSED(y)

  QMetaObject::invokeMethod(this, "CatchUpScrollToPoint", Qt::QueuedConnection,
                            Q_ARG(int, x));
}

void NodeParamView::UpdateElementY()
{
  for (auto it=items_.cbegin(); it!=items_.cend(); it++) {
    foreach (const QString& input, it.key()->inputs()) {
      int arr_sz = it.key()->InputArraySize(input);

      for (int i=-1; i<arr_sz; i++) {
        NodeInput ic = {it.key(), input, i};

        int y = it.value()->GetElementY(ic);
        keyframe_view_->SetElementY(ic, y);
      }
    }
  }
}

}

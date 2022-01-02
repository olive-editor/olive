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

#include "common/functiontimer.h"
#include "common/timecodefunctions.h"
#include "node/output/viewer/viewer.h"

namespace olive {

#define super TimeBasedWidget

NodeParamView::NodeParamView(bool create_keyframe_view, QWidget *parent) :
  super(true, false, parent),
  last_scroll_val_(0),
  focused_node_(nullptr),
  create_checkboxes_(kNoCheckBoxes),
  time_target_(nullptr),
  ignore_flags_(false)
{
  // Create horizontal layout to place scroll area in (and keyframe editing eventually)
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  // Set up scroll area for params
  param_scroll_area_ = new QScrollArea();
  param_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  param_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  param_scroll_area_->setWidgetResizable(true);
  splitter->addWidget(param_scroll_area_);

  // Param widget
  param_widget_container_ = new QWidget();
  param_scroll_area_->setWidget(param_widget_container_);

  param_widget_area_ = new NodeParamViewDockArea();

  QVBoxLayout* param_widget_container_layout = new QVBoxLayout(param_widget_container_);
  QMargins param_widget_margin = param_widget_container_layout->contentsMargins();
  param_widget_margin.setTop(ruler()->height());
  param_widget_container_layout->setContentsMargins(param_widget_margin);
  param_widget_container_layout->setSpacing(0);
  param_widget_container_layout->addWidget(param_widget_area_);

  param_widget_container_layout->addStretch(INT_MAX);

  // Create contexts for three different types
  context_items_.resize(Track::kCount + 1);
  for (int i=0; i<context_items_.size(); i++) {
    NodeParamViewContext *c = new NodeParamViewContext;
    c->setVisible(false);

    NodeParamViewItemTitleBar *title_bar = static_cast<NodeParamViewItemTitleBar*>(c->titleBarWidget());

    if (i == Track::kVideo || i == Track::kAudio) {
      title_bar->SetAddEffectButtonVisible(true);
      title_bar->SetText(tr("%1 Nodes").arg(Footage::GetStreamTypeName(static_cast<Track::Type>(i))));
    } else {
      title_bar->SetText(tr("Other"));
    }

    context_items_[i] = c;
    param_widget_area_->AddItem(c);
  }

  // Disable collapsing param view (but collapsing keyframe view is permitted)
  splitter->setCollapsible(0, false);

  // Create global vertical scrollbar on the right
  vertical_scrollbar_ = new QScrollBar();
  vertical_scrollbar_->setMaximum(0);
  layout->addWidget(vertical_scrollbar_);

  // Connect scrollbars together
  connect(param_scroll_area_->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
  connect(param_scroll_area_->verticalScrollBar(), &QScrollBar::rangeChanged, vertical_scrollbar_, &QScrollBar::setRange);
  connect(param_scroll_area_->verticalScrollBar(), &QScrollBar::rangeChanged, this, &NodeParamView::UpdateGlobalScrollBar);
  connect(vertical_scrollbar_, &QScrollBar::valueChanged, param_scroll_area_->verticalScrollBar(), &QScrollBar::setValue);

  if (create_keyframe_view) {
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
    connect(keyframe_view_, &KeyframeView::TimeChanged, this, &NodeParamView::SetTime);
    connect(keyframe_view_, &KeyframeView::Dragged, this, &NodeParamView::KeyframeViewDragged);

    // Connect keyframe view scaling to this
    connect(keyframe_view_, &KeyframeView::ScaleChanged, this, &NodeParamView::SetScale);

    splitter->addWidget(keyframe_area);

    // Set both widgets to 50/50
    splitter->setSizes({INT_MAX, INT_MAX});

    connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, vertical_scrollbar_, &QScrollBar::setValue);
    connect(keyframe_view_->verticalScrollBar(), &QScrollBar::valueChanged, param_scroll_area_->verticalScrollBar(), &QScrollBar::setValue);
    connect(param_scroll_area_->verticalScrollBar(), &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);
    connect(vertical_scrollbar_, &QScrollBar::valueChanged, keyframe_view_->verticalScrollBar(), &QScrollBar::setValue);

    // TimeBasedWidget's scrollbar has extra functionality that we can take advantage of
    keyframe_view_->setHorizontalScrollBar(scrollbar());
    keyframe_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(keyframe_view_->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);
  } else {
    keyframe_view_ = nullptr;
  }

  // Set a default scale - FIXME: Hardcoded
  SetScale(120);

  // Pickup on widget focus changes
  connect(qApp,
          &QApplication::focusChanged,
          this,
          &NodeParamView::FocusChanged);
}

NodeParamView::~NodeParamView()
{
  qDeleteAll(context_items_);
}

/*void NodeParamView::SelectNodes(const QVector<Node *> &nodes)
{
  return;
  int original_node_count = items_.size();

  foreach (Node* n, nodes) {
    // If we've already added this node (either a duplicate or a pinned node), don't add another
    if (items_.contains(n)) {
      continue;
    }

    // Add to "active" list to represent currently selected node
    active_nodes_.append(n);

    // Create node UI
    AddNode(n, param_widget_area_);
  }

  if (items_.size() > original_node_count ) {
    UpdateItemTime(GetTime());

    // Re-arrange keyframes
    QueueKeyframePositionUpdate();

    SignalNodeOrder();
  }
}

void NodeParamView::DeselectNodes(const QVector<Node *> &nodes)
{
  return;
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
}*/

void NodeParamView::SetInputChecked(const NodeInput &input, bool e)
{
  input_checked_.insert(input, e);
  foreach (NodeParamViewContext *ctx, context_items_) {
    ctx->SetInputChecked(input, e);
  }
}

void NodeParamView::SetContexts(const QVector<Node *> &contexts)
{
  TIME_THIS_FUNCTION;

  foreach (NodeParamViewContext *ctx, context_items_) {
    ctx->Clear();
    ctx->setVisible(false);
  }
  contexts_ = contexts;

  if (keyframe_view_) {
    keyframe_view_->Clear();
  }

  if (focused_node_) {
    focused_node_ = nullptr;
    emit FocusedNodeChanged(nullptr);
  }

  foreach (Node *ctx, contexts) {
    Track::Type ctx_type = Track::kCount;

    if (ClipBlock *clip = dynamic_cast<ClipBlock*>(ctx)) {
      if (clip->track()) {
        if (clip->track()->type() != Track::kNone) {
          ctx_type = clip->track()->type();
        }
      }
    } else if (Track *track = dynamic_cast<Track*>(ctx)) {
      if (track->type() != Track::kNone) {
        ctx_type = track->type();
      }
    }

    NodeParamViewContext *item = context_items_.at(ctx_type);

    item->AddContext(ctx);
    item->setVisible(true);

    for (auto it=ctx->GetContextPositions().cbegin(); it!=ctx->GetContextPositions().cend(); it++) {
      if (!(it.key()->GetFlags() & Node::kDontShowInParamView) || ignore_flags_) {
        AddNode(it.key(), item);
      }
    }
  }

  foreach (NodeParamViewContext *ctx, context_items_) {
    SortItemsInContext(ctx);
  }

  if (keyframe_view_) {
    QueueKeyframePositionUpdate();
  }
}

void NodeParamView::resizeEvent(QResizeEvent *event)
{
  super::resizeEvent(event);

  vertical_scrollbar_->setPageStep(vertical_scrollbar_->height());
}

void NodeParamView::ScaleChangedEvent(const double &scale)
{
  super::ScaleChangedEvent(scale);

  if (keyframe_view_) {
    keyframe_view_->SetScale(scale);
  }
}

void NodeParamView::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  if (keyframe_view_) {
    keyframe_view_->SetTimebase(timebase);
  }

  foreach (NodeParamViewContext* ctx, context_items_) {
    ctx->SetTimebase(timebase);
  }

  UpdateItemTime(GetTime());
}

void NodeParamView::TimeChangedEvent(const rational &time)
{
  super::TimeChangedEvent(time);

  if (keyframe_view_) {
    keyframe_view_->SetTime(time);
  }

  UpdateItemTime(time);
}

void NodeParamView::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  if (keyframe_view_) {
    // Set viewer as a time target
    keyframe_view_->SetTimeTarget(n);
  }

  foreach (NodeParamViewContext* item, context_items_) {
    item->SetTimeTarget(n);
  }

  time_target_ = n;
}

Node *NodeParamView::GetTimeTarget() const
{
  return time_target_;
}

void NodeParamView::DeleteSelected()
{
  if (keyframe_view_) {
    keyframe_view_->DeleteSelected();
  }
}

void NodeParamView::SelectNodes(const QVector<Node *> &nodes)
{
  // Do nothing, this is a placeholder if we ever need this to do anything in the future
}

void NodeParamView::DeselectNodes(const QVector<Node *> &nodes)
{
  // Do nothing, this is a placeholder if we ever need this to do anything in the future
}

void NodeParamView::UpdateItemTime(const rational &time)
{
  foreach (NodeParamViewContext* item, context_items_) {
    item->SetTime(time);
  }
}

void NodeParamView::QueueKeyframePositionUpdate()
{
  QMetaObject::invokeMethod(this, &NodeParamView::UpdateElementY, Qt::QueuedConnection);
}

void NodeParamView::AddNode(Node *n, NodeParamViewContext *context)
{
  NodeParamViewItem* item = new NodeParamViewItem(n, create_checkboxes_, context);

  connect(item, &NodeParamViewItem::RequestSetTime, this, &NodeParamView::SetTimeAndSignal);
  connect(item, &NodeParamViewItem::RequestSelectNode, this, &NodeParamView::RequestSelectNode);
  connect(item, &NodeParamViewItem::PinToggled, this, &NodeParamView::PinNode);
  connect(item, &NodeParamViewItem::InputCheckedChanged, this, &NodeParamView::SetInputChecked);

  if (create_checkboxes_) {
    for (auto it=input_checked_.cbegin(); it!=input_checked_.cend(); it++) {
      if (it.key().node() == n) {
        item->SetInputChecked(it.key(), it.value());
      }
    }
  }

  item->SetTimeTarget(GetTimeTarget());
  item->SetTimebase(timebase());
  item->SetTime(GetTime());

  context->AddNode(item);

  if (!focused_node_ && n->HasGizmos()) {
    // We'll focus this node now
    item->SetHighlighted(true);
    focused_node_ = item;
    emit FocusedNodeChanged(n);
  }

  if (keyframe_view_) {
    connect(item, &NodeParamViewItem::dockLocationChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
    connect(item, &NodeParamViewItem::ArrayExpandedChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
    connect(item, &NodeParamViewItem::ExpandedChanged, this, &NodeParamView::QueueKeyframePositionUpdate);
    connect(item, &NodeParamViewItem::Moved, this, &NodeParamView::QueueKeyframePositionUpdate);

    item->SetKeyframeConnections(keyframe_view_->AddKeyframesOfNode(n));
  }
}

int GetDistanceBetweenNodes(Node *start, Node *end)
{
  if (start == end) {
    return 0;
  }

  for (auto it=start->input_connections().cbegin(); it!=start->input_connections().cend(); it++) {
    int this_node_dist = GetDistanceBetweenNodes(it->second, end);
    if (this_node_dist != -1) {
      return 1 + this_node_dist;
    }
  }

  return -1;
}

void NodeParamView::SortItemsInContext(NodeParamViewContext *context_item)
{
  QVector<QPair<NodeParamViewItem*, int> > distances;

  for (auto it=context_item->GetItems().cbegin(); it!=context_item->GetItems().cend(); it++) {
    int distance = -1;
    foreach (Node *ctx, context_item->GetContexts()) {
      distance = qMax(distance, GetDistanceBetweenNodes(ctx, it.key()));
    }

    if (distance == -1) {
      distance = INT_MAX;
    }

    bool inserted = false;
    QPair<NodeParamViewItem*, int> dist(it.value(), distance);

    for (int i=0; i<distances.size(); i++) {
      if (distances.at(i).second < distance) {
        distances.insert(i, dist);
        inserted = true;
        break;
      }
    }

    if (!inserted) {
      distances.append(dist);
    }
  }

  foreach (auto info, distances) {
    context_item->GetDockArea()->AddItem(info.first);
  }
}

void NodeParamView::UpdateGlobalScrollBar()
{
  if (keyframe_view_) {
    keyframe_view_->SetMaxScroll(param_widget_container_->height() - ruler()->height());
  }
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
      //RemoveNode(node);
    }
  }
}

void NodeParamView::FocusChanged(QWidget* old, QWidget* now)
{
  Q_UNUSED(old)

  QObject* parent = now;

  while (parent) {
    if (NodeParamViewItem* item = dynamic_cast<NodeParamViewItem*>(parent)) {
      if (item != focused_node_) {
        // Found a NodeParamViewItem that isn't already focused, see if it belongs to us
        bool ours = false;

        do {
          parent = parent->parent();

          if (parent == this) {
            ours = true;
            break;
          }
        } while (parent);

        if (ours) {
          // This item is ours,
          if (focused_node_) {
            // De-focus current node
            focused_node_->SetHighlighted(false);
          }

          focused_node_ = item;

          item->SetHighlighted(true);

          emit FocusedNodeChanged(item->GetNode());
        }
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
  foreach (NodeParamViewContext *ctx, context_items_) {
    for (auto it=ctx->GetItems().cbegin(); it!=ctx->GetItems().cend(); it++) {
      const KeyframeView::NodeConnections &connections = it.value()->GetKeyframeConnections();

      if (!connections.isEmpty()) {
        foreach (const QString& input, it.key()->inputs()) {
          if (!(it.key()->GetInputFlags(input) & kInputFlagHidden)) {
            int arr_sz = it.key()->InputArraySize(input);

            for (int i=-1; i<arr_sz; i++) {
              NodeInput ic = {it.key(), input, i};

              int y = it.value()->GetElementY(ic);

              // For some reason Qt's mapToGlobal doesn't seem to handle this, so we offset here
              y += vertical_scrollbar_->value();

              const KeyframeView::InputConnections &input_con = connections.value(input);
              int use_index = i + 1;
              if (use_index < input_con.size()) {
                const KeyframeView::ElementConnections &ele_con = input_con.at(ic.element()+1);
                foreach (KeyframeViewInputConnection *track, ele_con) {
                  track->SetKeyframeY(y);
                }
              }
            }
          }
        }
      }
    }
  }
}

}

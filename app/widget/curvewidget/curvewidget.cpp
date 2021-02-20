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

#include "curvewidget.h"

#include <QEvent>
#include <QLabel>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

#include "core.h"
#include "common/qtutils.h"
#include "common/timecodefunctions.h"
#include "node/node.h"
#include "widget/keyframeview/keyframeviewundo.h"

namespace olive {

CurveWidget::CurveWidget(QWidget *parent) :
  TimeBasedWidget(parent)
{
  QHBoxLayout* outer_layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter();
  outer_layout->addWidget(splitter);

  tree_view_ = new NodeTreeView();
  tree_view_->SetOnlyShowKeyframable(true);
  tree_view_->SetShowKeyframeTracksAsRows(true);
  connect(tree_view_, &NodeTreeView::NodeEnableChanged, this, &CurveWidget::NodeEnabledChanged);
  connect(tree_view_, &NodeTreeView::InputEnableChanged, this, &CurveWidget::InputEnabledChanged);
  connect(tree_view_, &NodeTreeView::InputSelectionChanged, this, &CurveWidget::InputSelectionChanged);
  connect(tree_view_, &NodeTreeView::InputDoubleClicked, this, &CurveWidget::InputDoubleClicked);
  splitter->addWidget(tree_view_);

  QWidget* workarea = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(workarea);
  layout->setMargin(0);
  splitter->addWidget(workarea);

  QHBoxLayout* top_controls = new QHBoxLayout();

  key_control_ = new NodeParamViewKeyframeControl(false);
  connect(key_control_, &NodeParamViewKeyframeControl::RequestSetTime, this, &CurveWidget::KeyControlRequestedTimeChanged);
  top_controls->addWidget(key_control_);

  top_controls->addStretch();

  linear_button_ = new QPushButton(tr("Linear"));
  linear_button_->setCheckable(true);
  linear_button_->setEnabled(false);
  top_controls->addWidget(linear_button_);
  connect(linear_button_, &QPushButton::clicked, this, &CurveWidget::KeyframeTypeButtonTriggered);

  bezier_button_ = new QPushButton(tr("Bezier"));
  bezier_button_->setCheckable(true);
  bezier_button_->setEnabled(false);
  top_controls->addWidget(bezier_button_);
  connect(bezier_button_, &QPushButton::clicked, this, &CurveWidget::KeyframeTypeButtonTriggered);

  hold_button_ = new QPushButton(tr("Hold"));
  hold_button_->setCheckable(true);
  hold_button_->setEnabled(false);
  top_controls->addWidget(hold_button_);
  connect(hold_button_, &QPushButton::clicked, this, &CurveWidget::KeyframeTypeButtonTriggered);

  layout->addLayout(top_controls);

  // We use a separate layout for the ruler+view combination so that there's no spacing between them
  QVBoxLayout* ruler_view_layout = new QVBoxLayout();
  ruler_view_layout->setMargin(0);
  ruler_view_layout->setSpacing(0);

  ruler_view_layout->addWidget(ruler());

  view_ = new CurveView();
  ConnectTimelineView(view_);
  ruler_view_layout->addWidget(view_);

  layout->addLayout(ruler_view_layout);

  // Connect ruler and view together
  connect(view_, &CurveView::TimeChanged, this, &CurveWidget::SetTimeAndSignal);
  connect(view_->scene(), &QGraphicsScene::selectionChanged, this, &CurveWidget::SelectionChanged);
  connect(view_, &CurveView::ScaleChanged, this, &CurveWidget::SetScale);
  connect(view_, &CurveView::Dragged, this, &CurveWidget::KeyframeViewDragged);

  // TimeBasedWidget's scrollbar has extra functionality that we can take advantage of
  view_->setHorizontalScrollBar(scrollbar());
  connect(view_->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);

  // Disable collapsing the main curve view (but allow collapsing the tree)
  splitter->setCollapsible(1, false);

  SetScale(120.0);
}

CurveWidget::~CurveWidget()
{
  // Quick way to avoid segfault when QGraphicsScene::selectionChanged is emitted after other members have been destroyed
  view_->Clear();
}

const double &CurveWidget::GetVerticalScale()
{
  return view_->GetYScale();
}

void CurveWidget::SetVerticalScale(const double &vscale)
{
  view_->SetYScale(vscale);
}

void CurveWidget::DeleteSelected()
{
  view_->DeleteSelected();
}

void CurveWidget::SetNodes(const QVector<Node *> &nodes)
{
  tree_view_->SetNodes(nodes);

  // Detect removed nodes
  foreach (Node* n, nodes_) {
    if (!nodes.contains(n)) {
      ConnectNode(n, false);
    }
  }

  // Detect added nodes
  foreach (Node* n, nodes) {
    if (tree_view_->IsNodeEnabled(n) && !nodes_.contains(n)) {
      ConnectNode(n, true);
    }
  }

  // Save new node list
  nodes_ = nodes;
}

void CurveWidget::TimeChangedEvent(const int64_t &timestamp)
{
  TimeBasedWidget::TimeChangedEvent(timestamp);

  view_->SetTime(timestamp);
  UpdateBridgeTime(timestamp);
}

void CurveWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimeBasedWidget::TimebaseChangedEvent(timebase);

  view_->SetTimebase(timebase);
}

void CurveWidget::ScaleChangedEvent(const double &scale)
{
  TimeBasedWidget::ScaleChangedEvent(scale);

  view_->SetScale(scale);
}

void CurveWidget::TimeTargetChangedEvent(Node *target)
{
  key_control_->SetTimeTarget(target);

  view_->SetTimeTarget(target);
}

void CurveWidget::ConnectedNodeChanged(Sequence *n)
{
  SetTimeTarget(n);
}

void CurveWidget::SetKeyframeButtonEnabled(bool enable)
{
  linear_button_->setEnabled(enable);
  bezier_button_->setEnabled(enable);
  hold_button_->setEnabled(enable);
}

void CurveWidget::SetKeyframeButtonChecked(bool checked)
{
  linear_button_->setChecked(checked);
  bezier_button_->setChecked(checked);
  hold_button_->setChecked(checked);
}

void CurveWidget::SetKeyframeButtonCheckedFromType(NodeKeyframe::Type type)
{
  linear_button_->setChecked(type == NodeKeyframe::kLinear);
  bezier_button_->setChecked(type == NodeKeyframe::kBezier);
  hold_button_->setChecked(type == NodeKeyframe::kHold);
}

void CurveWidget::UpdateBridgeTime(const int64_t &timestamp)
{
  rational time = Timecode::timestamp_to_time(timestamp, view_->timebase());
  key_control_->SetTime(time);
}

void CurveWidget::ConnectNode(Node *node, bool connect)
{
  foreach (const QString& input, node->inputs()) {
    if (node->IsInputKeyframable(input)) {
      ConnectInput(node, input, connect);
    }
  }

  // Connect add/remove signals
  if (connect) {
    QObject::connect(node, &Node::KeyframeAdded, this, &CurveWidget::AddKeyframe);
    QObject::connect(node, &Node::KeyframeRemoved, this, &CurveWidget::RemoveKeyframe);
  } else {
    QObject::disconnect(node, &Node::KeyframeAdded, this, &CurveWidget::AddKeyframe);
    QObject::disconnect(node, &Node::KeyframeRemoved, this, &CurveWidget::RemoveKeyframe);
  }
}

void CurveWidget::ConnectInput(Node *node, const QString &input, bool connect)
{
  if (!node->IsInputKeyframable(input)) {
    qWarning() << "Tried to connect input that isn't keyframable";
    return;
  }

  int track_count = NodeValue::get_number_of_keyframe_tracks(node->GetInputDataType(input));
  bool multiple_tracks = track_count > 1;

  int arr_sz = node->InputArraySize(input);
  for (int i=-1; i<arr_sz; i++) {
    // Generate a random color for this input
    const QVector<NodeKeyframeTrack>& tracks = node->GetKeyframeTracks(input, i);

    for (int j=0; j<tracks.size(); j++) {
      NodeKeyframeTrackReference ref(NodeInput(node, input, i), j);

      if (!keyframe_colors_.contains(ref)) {
        QColor c = QColor::fromHsv(std::rand()%360, std::rand()%255, 255);

        keyframe_colors_.insert(ref, c);
        tree_view_->SetKeyframeTrackColor(ref, c);
        view_->SetKeyframeTrackColor(ref, c);
      }
    }

    if (tree_view_->IsInputEnabled(NodeKeyframeTrackReference(NodeInput(node, input, i), multiple_tracks ? -1 : 0))) {
      if (multiple_tracks) {
        for (int j=0; j<track_count; j++) {
          NodeKeyframeTrackReference ref(NodeInput(node, input, i), j);
          if (tree_view_->IsInputEnabled(ref)) {
            if (connect) {
              view_->ConnectInput(ref);
            } else {
              view_->DisconnectInput(ref);
            }
          }
        }
      } else {
        NodeKeyframeTrackReference ref(NodeInput(node, input, i), 0);
        if (connect) {
          view_->ConnectInput(ref);
        } else {
          view_->DisconnectInput(ref);
        }
      }
    }
  }
}

void CurveWidget::SelectionChanged()
{
  QList<QGraphicsItem*> selected = view_->scene()->selectedItems();

  SetKeyframeButtonChecked(false);
  SetKeyframeButtonEnabled(!selected.isEmpty());

  if (!selected.isEmpty()) {
    bool all_same_type = true;
    NodeKeyframe::Type type = static_cast<KeyframeViewItem*>(selected.first())->key()->type();

    for (int i=1;i<selected.size();i++) {
      KeyframeViewItem* prev_item = static_cast<KeyframeViewItem*>(selected.at(i-1));
      KeyframeViewItem* this_item = static_cast<KeyframeViewItem*>(selected.at(i));

      if (prev_item->key()->type() != this_item->key()->type()) {
        all_same_type = false;
        break;
      }
    }

    if (all_same_type) {
      SetKeyframeButtonCheckedFromType(type);
    }
  }
}

void CurveWidget::KeyframeTypeButtonTriggered(bool checked)
{
  QPushButton* key_btn = static_cast<QPushButton*>(sender());

  if (!checked) {
    // Keyframe buttons cannot be checked off, we undo this action here
    key_btn->setChecked(true);
    return;
  }

  // Get selected items and do nothing if there are none
  QList<QGraphicsItem*> selected = view_->scene()->selectedItems();
  if (selected.isEmpty()) {
    return;
  }

  // Set all selected keyframes to this type
  NodeKeyframe::Type new_type;

  // Determine which type to set
  if (key_btn == bezier_button_) {
    new_type = NodeKeyframe::kBezier;
  } else if (key_btn == hold_button_) {
    new_type = NodeKeyframe::kHold;
  } else {
    new_type = NodeKeyframe::kLinear;
  }

  // Ensure only the appropriate button is checked
  SetKeyframeButtonCheckedFromType(new_type);

  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (QGraphicsItem* item, selected) {
    KeyframeViewItem* key_item = static_cast<KeyframeViewItem*>(item);

    command->add_child(new KeyframeSetTypeCommand(key_item->key(), new_type));
  }

  Core::instance()->undo_stack()->push(command);
}

void CurveWidget::KeyControlRequestedTimeChanged(const rational &time)
{
  SetTimeAndSignal(Timecode::time_to_timestamp(time, view_->timebase()));
}

void CurveWidget::NodeEnabledChanged(Node* n, bool e)
{
  ConnectNode(n, e);
}

void CurveWidget::InputEnabledChanged(const NodeKeyframeTrackReference& ref, bool e)
{
  if (e) {
    view_->ConnectInput(ref);
  } else {
    view_->DisconnectInput(ref);
  }
}

void CurveWidget::AddKeyframe(NodeKeyframe *key)
{
  view_->AddKeyframe(key);
}

void CurveWidget::RemoveKeyframe(NodeKeyframe *key)
{
  view_->RemoveKeyframe(key);
}

void CurveWidget::InputSelectionChanged(const NodeKeyframeTrackReference& ref)
{
  key_control_->SetInput(ref.input());

  if (ref.IsValid()) {
    view_->SelectKeyframesOfInput(ref);
  }
}

void CurveWidget::InputDoubleClicked(const NodeKeyframeTrackReference& ref)
{
  view_->ZoomToFitInput(ref);
}

void CurveWidget::KeyframeViewDragged(int x, int y)
{
  QMetaObject::invokeMethod(this, "CatchUpScrollToPoint", Qt::QueuedConnection,
                            Q_ARG(int, x));
  QMetaObject::invokeMethod(this, "CatchUpYScrollToPoint", Qt::QueuedConnection,
                            Q_ARG(int, y));
}

void CurveWidget::CatchUpYScrollToPoint(int point)
{
  PageScrollInternal(view_->verticalScrollBar(), view_->height(), point, false);
}

}

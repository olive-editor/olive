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

#define super TimeBasedWidget

CurveWidget::CurveWidget(QWidget *parent) :
  super(parent)
{
  QHBoxLayout* outer_layout = new QHBoxLayout(this);

  QSplitter* splitter = new QSplitter();
  outer_layout->addWidget(splitter);

  tree_view_ = new NodeTreeView();
  tree_view_->SetOnlyShowKeyframable(true);
  tree_view_->SetShowKeyframeTracksAsRows(true);
  connect(tree_view_, &NodeTreeView::InputSelectionChanged, this, &CurveWidget::InputSelectionChanged);
  splitter->addWidget(tree_view_);

  QWidget* workarea = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(workarea);
  layout->setMargin(0);
  splitter->addWidget(workarea);

  QHBoxLayout* top_controls = new QHBoxLayout();

  key_control_ = new NodeParamViewKeyframeControl(false);
  connect(key_control_, &NodeParamViewKeyframeControl::RequestSetTime, this, &CurveWidget::SetTimeAndSignal);
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
  connect(view_, &CurveView::SelectionChanged, this, &CurveWidget::SelectionChanged);
  connect(view_, &CurveView::ScaleChanged, this, &CurveWidget::SetScale);
  connect(view_, &CurveView::Dragged, this, &CurveWidget::KeyframeViewDragged);

  // TimeBasedWidget's scrollbar has extra functionality that we can take advantage of
  view_->setHorizontalScrollBar(scrollbar());
  connect(view_->horizontalScrollBar(), &QScrollBar::valueChanged, ruler(), &TimeRuler::SetScroll);

  // Disable collapsing the main curve view (but allow collapsing the tree)
  splitter->setCollapsible(1, false);

  SetScale(120.0);
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

  // Save new node list
  nodes_ = nodes;

  // Generate colors
  foreach (Node *node, nodes_) {
    foreach (const QString& input, node->inputs()) {
      if (node->IsInputKeyframable(input) && !node->IsInputHidden(input)) {
        int arr_sz = node->InputArraySize(input);
        for (int i=-1; i<arr_sz; i++) {
          // Generate a random color for this input
          const QVector<NodeKeyframeTrack>& tracks = node->GetKeyframeTracks(input, i);

          for (int j=0; j<tracks.size(); j++) {
            NodeKeyframeTrackReference ref(NodeInput(node, input, i), j);

            if (!keyframe_colors_.contains(ref)) {
              QColor c = QColor::fromHsl(std::rand()%360, 255, 160);

              keyframe_colors_.insert(ref, c);
              tree_view_->SetKeyframeTrackColor(ref, c);
              view_->SetKeyframeTrackColor(ref, c);
            }
          }
        }
      }
    }
  }
}

void CurveWidget::TimeChangedEvent(const rational &time)
{
  super::TimeChangedEvent(time);

  view_->SetTime(time);
  UpdateBridgeTime(time);
}

void CurveWidget::TimebaseChangedEvent(const rational &timebase)
{
  super::TimebaseChangedEvent(timebase);

  view_->SetTimebase(timebase);
}

void CurveWidget::ScaleChangedEvent(const double &scale)
{
  super::ScaleChangedEvent(scale);

  view_->SetScale(scale);
}

void CurveWidget::TimeTargetChangedEvent(Node *target)
{
  TimeTargetObject::TimeTargetChangedEvent(target);

  key_control_->SetTimeTarget(target);

  view_->SetTimeTarget(target);
}

void CurveWidget::ConnectedNodeChangeEvent(ViewerOutput *n)
{
  super::ConnectedNodeChangeEvent(n);

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

void CurveWidget::UpdateBridgeTime(const rational &time)
{
  key_control_->SetTime(time);
}

void CurveWidget::ConnectInput(Node *node, const QString &input, int element)
{
  if (element == -1 && node->InputIsArray(input)) {
    // This is the root element, connect all elements (if applicable)
    int arr_sz = node->InputArraySize(input);
    for (int i=-1; i<arr_sz; i++) {
      ConnectInputInternal(node, input, i);
    }
  } else {
    // This is a single element, just connect it as-is
    ConnectInputInternal(node, input, element);
  }
}

void CurveWidget::ConnectInputInternal(Node *node, const QString &input, int element)
{
  NodeInput input_ref(node, input, element);
  int track_count = NodeValue::get_number_of_keyframe_tracks(input_ref.GetDataType());
  for (int i=0; i<track_count; i++) {
    NodeKeyframeTrackReference track_ref(input_ref, i);
    view_->ConnectInput(track_ref);
    selected_tracks_.append(track_ref);
  }
}

void CurveWidget::SelectionChanged()
{
  const QVector<NodeKeyframe*> &selected = view_->GetSelectedKeyframes();

  SetKeyframeButtonChecked(false);
  SetKeyframeButtonEnabled(!selected.isEmpty());

  if (!selected.isEmpty()) {
    bool all_same_type = true;
    NodeKeyframe::Type type = selected.first()->type();

    for (int i=1;i<selected.size();i++) {
      NodeKeyframe* prev_item = selected.at(i-1);
      NodeKeyframe* this_item = selected.at(i);

      if (prev_item->type() != this_item->type()) {
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
  const QVector<NodeKeyframe*> &selected = view_->GetSelectedKeyframes();
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

  foreach (NodeKeyframe* item, selected) {
    command->add_child(new KeyframeSetTypeCommand(item, new_type));
  }

  Core::instance()->undo_stack()->push(command);
}

void CurveWidget::InputSelectionChanged(const NodeKeyframeTrackReference& ref)
{
  key_control_->SetInput(ref.input());

  foreach (const NodeKeyframeTrackReference &c, selected_tracks_) {
    view_->DisconnectInput(c);
  }

  selected_tracks_.clear();

  if (ref.IsValid() && !ref.input().IsArray()) {
    // This reference is a track, connect it only
    view_->ConnectInput(ref);
    selected_tracks_.append(ref);
  } else if (ref.input().IsValid()) {
    // This reference is a input, connect all tracks
    ConnectInput(ref.input().node(), ref.input().input(), ref.input().element());
  } else if (Node *node = ref.input().node()) {
    // This is a node, add all inputs
    foreach (const QString &input, node->inputs()) {
      if (node->IsInputKeyframable(input) && !node->IsInputHidden(input)) {
        ConnectInput(node, input, -1);
      }
    }
  }

  view_->ZoomToFit();
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

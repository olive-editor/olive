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

#include "curveview.h"

#include <cfloat>
#include <QHash>
#include <QMouseEvent>
#include <QPainterPath>
#include <QScrollBar>
#include <QtMath>

#include "common/qtutils.h"
#include "widget/keyframeview/keyframeviewundo.h"
#include "widget/nodeparamview/nodeparamviewundo.h"

namespace olive {

#define super KeyframeView

CurveView::CurveView(QWidget *parent) :
  KeyframeView(parent),
  dragging_bezier_pt_(nullptr)
{
  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  SetYAxisEnabled(true);
  SetAutoSelectSiblings(false);

  text_padding_ = QtUtils::QFontMetricsWidth(fontMetrics(), QStringLiteral("i"));

  minimum_grid_space_ = QtUtils::QFontMetricsWidth(fontMetrics(), QStringLiteral("00000"));
}

void CurveView::ConnectInput(const NodeKeyframeTrackReference& ref)
{
  if (connected_inputs_.contains(ref)) {
    // Input wasn't connected, do nothing
    return;
  }

  // Add keyframes from track
  KeyframeViewInputConnection *track_con = AddKeyframesOfTrack(ref);
  track_con->SetBrush(keyframe_colors_.value(ref));
  track_connections_.insert(ref, track_con);

  // Signal to CurveWidget to update its bezier/linear/hold buttons if a key type changes
  connect(track_con, &KeyframeViewInputConnection::TypeChanged, this, &CurveView::SelectionChanged);

  // Append to the list
  connected_inputs_.append(ref);
}

void CurveView::DisconnectInput(const NodeKeyframeTrackReference& ref)
{
  if (!connected_inputs_.contains(ref)) {
    // Input wasn't connected, do nothing
    return;
  }

  // Remove keyframes belonging to this element and track
  RemoveKeyframesOfTrack(track_connections_.take(ref));

  // Remove from the list
  connected_inputs_.removeOne(ref);
}

void CurveView::SelectKeyframesOfInput(const NodeKeyframeTrackReference& ref)
{
  DeselectAll();

  foreach (KeyframeViewInputConnection *con, track_connections_) {
    foreach (NodeKeyframe *key, con->GetKeyframes()) {
      SelectKeyframe(key);
    }
  }
}

void CurveView::SetKeyframeTrackColor(const NodeKeyframeTrackReference &ref, const QColor &color)
{
  // Insert color into hashmap
  keyframe_colors_.insert(ref, color);

  if (KeyframeViewInputConnection *con = track_connections_.value(ref)) {
    // Update all keyframes
    con->SetBrush(color);
  }
}

void CurveView::drawBackground(QPainter *painter, const QRectF &rect)
{
  if (timebase().isNull()) {
    return;
  }

  painter->setRenderHint(QPainter::Antialiasing);

  QVector<QLine> lines;

  double x_interval = timebase().flipped().toDouble();
  double y_interval = 100.0;

  int x_grid_interval, y_grid_interval;

  painter->setPen(QPen(palette().window().color(), 1));

  do {
    x_grid_interval = qRound(x_interval * GetScale() * timebase_dbl());
    x_interval *= 2.0;
  } while (x_grid_interval < minimum_grid_space_);

  do {
    y_grid_interval = qRound(y_interval * GetYScale());
    y_interval *= 2.0;
  } while (y_grid_interval < minimum_grid_space_);

  int x_start = qCeil(rect.left() / x_grid_interval) * x_grid_interval;
  int y_start = qCeil(rect.top() / y_grid_interval) * y_grid_interval;

  QPointF scene_bottom_left = mapToScene(QPoint(0, qRound(rect.height())));
  QPointF scene_top_right = mapToScene(QPoint(qRound(rect.width()), 0));

  // Add vertical lines
  for (int i=x_start;i<rect.right();i+=x_grid_interval) {
    int value = qRound(static_cast<double>(i) / GetScale() / timebase_dbl());
    painter->drawText(i + text_padding_, qRound(scene_bottom_left.y()) - text_padding_, QString::number(value));
    lines.append(QLine(i, qRound(rect.top()), i, qRound(rect.bottom())));
  }

  // Add horizontal lines
  for (int i=y_start;i<rect.bottom();i+=y_grid_interval) {
    int value = qRound(static_cast<double>(i) / GetYScale());
    painter->drawText(qRound(scene_bottom_left.x()) + text_padding_, i - text_padding_, QString::number(-value));
    lines.append(QLine(qRound(rect.left()), i, qRound(rect.right()), i));
  }

  // Draw grid
  painter->drawLines(lines);

  // Draw keyframe lines
  foreach (const NodeKeyframeTrackReference& ref, connected_inputs_) {
    Node* node = ref.input().node();
    const QString& input = ref.input().input();

    if (node->IsInputKeyframing(input, ref.input().element())) {
      const QVector<NodeKeyframeTrack>& tracks = node->GetKeyframeTracks(ref.input());

      const NodeKeyframeTrack& track = tracks.at(ref.track());

      if (!track.isEmpty()) {
        painter->setPen(QPen(keyframe_colors_.value(ref),
                             qMax(1, fontMetrics().height() / 4)));

        // Create a path
        QPainterPath path;

        // Draw straight line leading to first keyframe
        QPointF first_key_pos = GetKeyframePosition(track.first());
        path.moveTo(QPointF(scene_bottom_left.x(), first_key_pos.y()));
        path.lineTo(first_key_pos);

        // Draw lines between each keyframe
        for (int i=1;i<track.size();i++) {
          NodeKeyframe* before = track.at(i-1);
          NodeKeyframe* after = track.at(i);

          QPointF before_pos = GetKeyframePosition(before);
          QPointF after_pos = GetKeyframePosition(after);

          if (before->type() == NodeKeyframe::kHold) {

            // Draw a hold keyframe (basically a right angle)
            path.lineTo(after_pos.x(), before_pos.y());
            path.lineTo(after_pos.x(), after_pos.y());

          } else if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {

            // Draw a cubic bezier

            // Cubic beziers have two control points, so we can just use both
            QPointF before_control_point = before_pos + ScalePoint(before->valid_bezier_control_out());
            QPointF after_control_point = after_pos + ScalePoint(after->valid_bezier_control_in());

            path.cubicTo(before_control_point, after_control_point, after_pos);

          } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
            // Draw a quadratic bezier

            // Quadratic beziers have a single control point, we just have to determine which it is
            QPointF key_anchor;
            QPointF control_point;

            if (before->type() == NodeKeyframe::kBezier) {
              key_anchor = before_pos;
              control_point = before->valid_bezier_control_out();
            } else {
              key_anchor = after_pos;
              control_point = after->valid_bezier_control_in();
            }

            // Scale control point
            control_point = key_anchor + ScalePoint(control_point);

            // Create the path from both keyframes
            path.quadTo(control_point, after_pos);

          } else {

            // Linear to linear
            path.lineTo(after_pos);

          }
        }

        // Draw straight line leading from end keyframe
        QPointF last_key_pos = GetKeyframePosition(track.last());
        path.lineTo(QPointF(scene_top_right.x(), last_key_pos.y()));

        painter->drawPath(path);
      }
    }
  }
}

void CurveView::drawForeground(QPainter *painter, const QRectF &rect)
{
  bezier_pts_.clear();

  super::drawForeground(painter, rect);
}

void CurveView::ContextMenuEvent(Menu &m)
{
  // View settings
  QAction* zoom_fit_action = m.addAction(tr("Zoom to Fit"));
  connect(zoom_fit_action, &QAction::triggered, this, &CurveView::ZoomToFit);

  QAction* zoom_fit_selected_action = m.addAction(tr("Zoom to Fit Selected"));
  connect(zoom_fit_selected_action, &QAction::triggered, this, &CurveView::ZoomToFitSelected);

  QAction* reset_zoom_action = m.addAction(tr("Reset Zoom"));
  connect(reset_zoom_action, &QAction::triggered, this, &CurveView::ResetZoom);
}

void CurveView::SceneRectUpdateEvent(QRectF &r)
{
  double min_val, max_val;
  bool got_val = false;

  foreach (KeyframeViewInputConnection *con, track_connections_) {
    foreach (NodeKeyframe *key, con->GetKeyframes()) {
      qreal key_y = GetItemYFromKeyframeValue(key);

      if (got_val) {
        min_val = qMin(key_y, min_val);
        max_val = qMax(key_y, max_val);
      } else {
        min_val = key_y;
        max_val = key_y;
        got_val = true;
      }
    }
  }

  if (got_val) {
    r.setTop(min_val - this->height());
    r.setBottom(max_val + this->height());
  }
}

qreal CurveView::GetKeyframeSceneY(KeyframeViewInputConnection *track, NodeKeyframe *key)
{
  return GetItemYFromKeyframeValue(key);
}

void CurveView::DrawKeyframe(QPainter *painter, NodeKeyframe *key, KeyframeViewInputConnection *track, const QRectF &key_rect)
{
  if (IsKeyframeSelected(key) && key->type() == NodeKeyframe::kBezier) {
    // Draw bezier control points if keyframe is selected
    int control_point_size = QtUtils::QFontMetricsWidth(fontMetrics(), "o");
    int half_sz = control_point_size / 2;
    QRectF control_point_rect(-half_sz, -half_sz, control_point_size, control_point_size);

    painter->setPen(palette().text().color());
    painter->setBrush(Qt::NoBrush);

    QRectF cp_in = control_point_rect.translated(key_rect.center() + ScalePoint(key->bezier_control_in()));
    QRectF cp_out = control_point_rect.translated(key_rect.center() + ScalePoint(key->bezier_control_out()));

    painter->drawLine(key_rect.center(), cp_in.center());
    painter->drawLine(key_rect.center(), cp_out.center());

    painter->drawEllipse(cp_in);
    painter->drawEllipse(cp_out);

    bezier_pts_.append({cp_in, key, NodeKeyframe::kInHandle});
    bezier_pts_.append({cp_out, key, NodeKeyframe::kOutHandle});
  }

  super::DrawKeyframe(painter, key, track, key_rect);
}

bool CurveView::FirstChanceMousePress(QMouseEvent *event)
{
  dragging_bezier_pt_ = nullptr;
  QPointF scene_pt = mapToScene(event->pos());
  foreach (const BezierPoint &b, bezier_pts_) {
    if (b.rect.contains(scene_pt)) {
      dragging_bezier_pt_ = &b;
      break;
    }
  }

  if (dragging_bezier_pt_) {
    NodeKeyframe *key = dragging_bezier_pt_->keyframe;
    dragging_bezier_point_start_ = (dragging_bezier_pt_->type == NodeKeyframe::kInHandle) ? key->bezier_control_in() : key->bezier_control_out();
    dragging_bezier_point_opposing_start_ = (dragging_bezier_pt_->type == NodeKeyframe::kInHandle) ? key->bezier_control_out() : key->bezier_control_in();

    drag_start_ = mapToScene(event->pos());
    return true;
  } else {
    return false;
  }
}

void CurveView::FirstChanceMouseMove(QMouseEvent *event)
{
  // Calculate cursor difference and scale it
  QPointF scene_pos = mapToScene(event->pos());
  QPointF mouse_diff_scaled = GetScaledCursorPos(scene_pos - drag_start_);

  if (event->modifiers() & Qt::ShiftModifier) {
    // If holding shift, only move one axis
    mouse_diff_scaled.setY(0);
  }

  // Flip the mouse Y because bezier control points are drawn bottom to top, not top to bottom
  mouse_diff_scaled.setY(-mouse_diff_scaled.y());

  QPointF new_bezier_pos = GenerateBezierControlPosition(dragging_bezier_pt_->type,
                                                         dragging_bezier_point_start_,
                                                         mouse_diff_scaled);

  // If the user is NOT holding control, we set the other handle to the exact negative of this handle
  QPointF new_opposing_pos;
  NodeKeyframe::BezierType opposing_type = NodeKeyframe::get_opposing_bezier_type(dragging_bezier_pt_->type);


  if (!(event->modifiers() & Qt::ControlModifier)) {
    new_opposing_pos = GenerateBezierControlPosition(opposing_type,
                                                     dragging_bezier_point_opposing_start_,
                                                     -mouse_diff_scaled);
  } else {
    new_opposing_pos = dragging_bezier_point_opposing_start_;
  }

  dragging_bezier_pt_->keyframe->set_bezier_control(dragging_bezier_pt_->type,
                                                    new_bezier_pos);

  dragging_bezier_pt_->keyframe->set_bezier_control(opposing_type,
                                                    new_opposing_pos);

  Redraw();
}

void CurveView::FirstChanceMouseRelease(QMouseEvent *event)
{
  MultiUndoCommand* command = new MultiUndoCommand();

  // Create undo command with the current bezier point and the old one
  command->add_child(new KeyframeSetBezierControlPoint(dragging_bezier_pt_->keyframe,
                                                       dragging_bezier_pt_->type,
                                                       dragging_bezier_pt_->keyframe->bezier_control(dragging_bezier_pt_->type),
                                                       dragging_bezier_point_start_));

  if (!(event->modifiers() & Qt::ControlModifier)) {
    auto opposing_type = NodeKeyframe::get_opposing_bezier_type(dragging_bezier_pt_->type);

    command->add_child(new KeyframeSetBezierControlPoint(dragging_bezier_pt_->keyframe,
                                                         opposing_type,
                                                         dragging_bezier_pt_->keyframe->bezier_control(opposing_type),
                                                         dragging_bezier_point_opposing_start_));
  }

  dragging_bezier_pt_ = nullptr;

  Core::instance()->undo_stack()->push(command);
}

void CurveView::KeyframeDragStart(QMouseEvent *event)
{
  drag_keyframe_values_.resize(GetSelectedKeyframes().size());
  for (int i=0; i<GetSelectedKeyframes().size(); i++) {
    NodeKeyframe *key = GetSelectedKeyframes().at(i);
    drag_keyframe_values_[i] = key->value();
  }

  drag_start_ = mapToScene(event->pos());
}

void CurveView::KeyframeDragMove(QMouseEvent *event, QString &tip)
{
  if (event->modifiers() & Qt::ShiftModifier) {
    // Lock to X axis only and set original values on all keys
    for (int i=0; i<GetSelectedKeyframes().size(); i++) {
      NodeKeyframe *key = GetSelectedKeyframes().at(i);
      key->set_value(drag_keyframe_values_.at(i));
    }
    return;
  }

  // Calculate cursor difference
  double scaled_diff = (mapToScene(event->pos()).y() - drag_start_.y()) / GetYScale();

  // Validate movement - ensure no keyframe goes above its max point or below its min point
  for (int i=0; i<GetSelectedKeyframes().size(); i++) {
    NodeKeyframe *key = GetSelectedKeyframes().at(i);

    FloatSlider::DisplayType display = GetFloatDisplayTypeFromKeyframe(key);
    Node* node = key->parent();
    double original_val = FloatSlider::TransformValueToDisplay(drag_keyframe_values_.at(i).toDouble(), display);
    const QString& input = key->input();
    double new_val = FloatSlider::TransformDisplayToValue(original_val - scaled_diff, display);
    double limited = new_val;

    if (node->HasInputProperty(input, QStringLiteral("min"))) {
      limited = qMax(limited, node->GetInputProperty(input, QStringLiteral("min")).toDouble());
    }

    if (node->HasInputProperty(input, QStringLiteral("max"))) {
      limited = qMin(limited, node->GetInputProperty(input, QStringLiteral("max")).toDouble());
    }

    if (limited != new_val) {
      scaled_diff = original_val - limited;
    }
  }

  // Set values
  for (int i=0; i<GetSelectedKeyframes().size(); i++) {
    NodeKeyframe *key = GetSelectedKeyframes().at(i);
    FloatSlider::DisplayType display = GetFloatDisplayTypeFromKeyframe(key);
    key->set_value(FloatSlider::TransformDisplayToValue(FloatSlider::TransformValueToDisplay(drag_keyframe_values_.at(i).toDouble(), display) - scaled_diff, display));
  }

  NodeKeyframe *tip_item = GetSelectedKeyframes().first();

  bool ok;
  double num_value = tip_item->value().toDouble(&ok);

  if (ok) {
    tip = QStringLiteral("%1\n");
    tip.append(FloatSlider::ValueToString(num_value + GetOffsetFromKeyframe(tip_item), GetFloatDisplayTypeFromKeyframe(tip_item), 2, true));
  }
}

void CurveView::KeyframeDragRelease(QMouseEvent *event, MultiUndoCommand *command)
{
  for (int i=0; i<GetSelectedKeyframes().size(); i++) {
    NodeKeyframe *k = GetSelectedKeyframes().at(i);
    if (!qFuzzyCompare(k->value().toDouble(), drag_keyframe_values_.at(i).toDouble())) {
      command->add_child(new NodeParamSetKeyframeValueCommand(k, k->value(), drag_keyframe_values_.at(i)));
    }
  }
}

QPointF CurveView::GenerateBezierControlPosition(const NodeKeyframe::BezierType mode, const QPointF &start_point, const QPointF &scaled_cursor_diff)
{
  QPointF new_bezier_pos = start_point;

  new_bezier_pos += scaled_cursor_diff;

  // LIMIT bezier handles from overlapping each other
  if (mode == NodeKeyframe::kInHandle) {
    if (new_bezier_pos.x() > 0) {
      new_bezier_pos.setX(0);
    }
  } else {
    if (new_bezier_pos.x() < 0) {
      new_bezier_pos.setX(0);
    }
  }

  return new_bezier_pos;
}

QPointF CurveView::GetScaledCursorPos(const QPointF &cursor_pos)
{
  return QPointF(cursor_pos.x() / GetScale(),
                 cursor_pos.y() / GetYScale());
}

void CurveView::ZoomToFitInternal(bool selected_only)
{
  bool got_val = false;

  rational min_time, max_time;
  double min_val, max_val;

  foreach (KeyframeViewInputConnection *con, track_connections_) {
    foreach (NodeKeyframe *key, con->GetKeyframes()) {
      if (!selected_only || IsKeyframeSelected(key)) {
        rational transformed_time = GetAdjustedTime(key->parent(),
                                                    GetTimeTarget(),
                                                    key->time(),
                                                    false);

        qreal key_y = GetUnscaledItemYFromKeyframeValue(key);

        if (got_val) {
          min_time = qMin(transformed_time, min_time);
          max_time = qMax(transformed_time, max_time);

          min_val = qMin(key_y, min_val);
          max_val = qMax(key_y, max_val);
        } else {
          min_time = transformed_time;
          max_time = transformed_time;

          min_val = key_y;
          max_val = key_y;

          got_val = true;
        }
      }
    }
  }

  // Prevent scaling if no keyframes were found
  if (got_val) {
    QRectF desired(QPointF(min_time.toDouble(), min_val), QPointF(max_time.toDouble(), max_val));

    const double scale_divider = 0.5;
    double scale_half_divider = scale_divider*0.5;

    double new_x_scale = viewport()->width() / desired.width() * scale_divider;
    double new_y_scale;

    if (qFuzzyIsNull(desired.height())) {
      // Catch divide by zero
      new_y_scale = 1.0;
      scale_half_divider = 0.5;
    } else {
      // Use height as normal
      new_y_scale = viewport()->height() / desired.height() * scale_divider;
    }

    emit ScaleChanged(new_x_scale);
    SetYScale(new_y_scale);

    UpdateSceneRect();

    int sb_x = desired.left() * new_x_scale - viewport()->width() * scale_half_divider;
    QMetaObject::invokeMethod(horizontalScrollBar(), "setValue", Qt::QueuedConnection, Q_ARG(int, sb_x));

    int sb_y = desired.top() * new_y_scale - viewport()->height() * scale_half_divider;
    QMetaObject::invokeMethod(verticalScrollBar(), "setValue", Qt::QueuedConnection, Q_ARG(int, sb_y));
  }
}

qreal CurveView::GetItemYFromKeyframeValue(NodeKeyframe *key)
{
  return GetUnscaledItemYFromKeyframeValue(key) * GetYScale();
}

qreal CurveView::GetUnscaledItemYFromKeyframeValue(NodeKeyframe *key)
{
  double val = key->value().toDouble();

  val = FloatSlider::TransformValueToDisplay(val, GetFloatDisplayTypeFromKeyframe(key));

  val += GetOffsetFromKeyframe(key);

  return -val;
}

QPointF CurveView::ScalePoint(const QPointF &point)
{
  // Flips Y coordinate because curves are drawn bottom to top
  return QPointF(point.x() * GetScale(), - point.y() * GetYScale());
}

FloatSlider::DisplayType CurveView::GetFloatDisplayTypeFromKeyframe(NodeKeyframe *key)
{
  Node* node = key->parent();
  const QString& input = key->input();
  if (node->HasInputProperty(input, QStringLiteral("view"))) {
    // Try to get view from input (which will be normal if unset)
    return static_cast<FloatSlider::DisplayType>(node->GetInputProperty(input, QStringLiteral("view")).toInt());
  }

  // Fallback to normal
  return FloatSlider::kNormal;
}

double CurveView::GetOffsetFromKeyframe(NodeKeyframe *key)
{
  Node *node = key->parent();
  const QString &input = key->input();
  if (node->HasInputProperty(input, QStringLiteral("offset"))) {
    QVariant v = node->GetInputProperty(input, QStringLiteral("offset"));

    // NOTE: Implement getting correct offset for the track based on the data type
    QVector<QVariant> track_vals = NodeValue::split_normal_value_into_track_values(node->GetInputDataType(input), v);

    return track_vals.at(key->track()).toDouble();
  }

  return 0;
}

QPointF CurveView::GetKeyframePosition(NodeKeyframe *key)
{
  return QPointF(GetKeyframeSceneX(key), GetItemYFromKeyframeValue(key));
}

void CurveView::ZoomToFit()
{
  ZoomToFitInternal(false);
}

void CurveView::ZoomToFitSelected()
{
  ZoomToFitInternal(true);
}

void CurveView::ResetZoom()
{
  emit ScaleChanged(1.0);
  SetYScale(1.0);
}

}

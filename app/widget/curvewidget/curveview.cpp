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

#include "curveview.h"

#include <QScrollBar>
#include <QMouseEvent>
#include <QtMath>
#include <cfloat>

#include "common/qtutils.h"

OLIVE_NAMESPACE_ENTER

CurveView::CurveView(QWidget *parent) :
  KeyframeViewBase(parent)
{
  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  setDragMode(RubberBandDrag);
  setViewportUpdateMode(FullViewportUpdate);
  SetYAxisEnabled(true);

  text_padding_ = QFontMetricsWidth(fontMetrics(), QStringLiteral("i"));

  minimum_grid_space_ = QFontMetricsWidth(fontMetrics(), QStringLiteral("00000"));

  connect(scene(), &QGraphicsScene::selectionChanged, this, &CurveView::SelectionChanged);
}

CurveView::~CurveView()
{
  // Quick way to avoid segfault when QGraphicsScene::selectionChanged is emitted after other members have been destroyed
  Clear();
}

void CurveView::Clear()
{
  KeyframeViewBase::Clear();

  foreach (QGraphicsLineItem* line, lines_) {
    delete line;
  }
  lines_.clear();
}

void CurveView::SetTrackCount(int count)
{
  track_count_ = count;

  track_visible_.resize(track_count_);
  track_visible_.fill(true);
}

void CurveView::SetTrackVisible(int track, bool visible)
{
  track_visible_[track] = visible;

  SetKeyframeTrackVisible(track, visible);
}

void CurveView::ConnectInput(NodeInput *input)
{
  if (connected_inputs_.contains(input)) {
    // Input wasn't connected, do nothing
    return;
  }

  // Add keyframes from this input
  foreach (const NodeInput::KeyframeTrack& track, input->keyframe_tracks()) {
    foreach (NodeKeyframePtr key, track) {
      this->AddKeyframe(key);
    }
  }

  // Append to the list
  connected_inputs_.append(input);

  // Connect add/remove signals
  connect(input, &NodeInput::KeyframeAdded, this, &CurveView::AddKeyframe);
  connect(input, &NodeInput::KeyframeRemoved, this, &CurveView::RemoveKeyframe);
}

void CurveView::DisconnectNode(Node *node)
{
  QList<NodeInput*> inputs = node->GetInputsIncludingArrays();

  foreach (NodeInput* i, inputs) {
    DisconnectInput(i);
  }
}

void CurveView::DisconnectInput(NodeInput *input)
{
  if (!connected_inputs_.contains(input)) {
    // Input wasn't connected, do nothing
    return;
  }

  // Remove keyframes belonging to this input
  RemoveKeyframesOfInput(input);

  // Remove from the list
  connected_inputs_.removeOne(input);

  // Disconnect add/remove signals
  disconnect(input, &NodeInput::KeyframeAdded, this, &CurveView::AddKeyframe);
  disconnect(input, &NodeInput::KeyframeRemoved, this, &CurveView::RemoveKeyframe);
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

  for (int j=0;j<track_count_;j++) {
    if (!track_visible_.at(j)) {
      continue;
    }

    painter->setPen(QPen(GetKeyframeColor(j), qMax(1, fontMetrics().height() / 4)));
    QList<NodeKeyframe*> keys = GetKeyframesSortedByTime(j);

    if (!keys.isEmpty()) {
      QVector<QLineF> keyframe_lines;

      // Draw straight line leading to first keyframe
      QPointF first_key_pos = item_map().value(keys.first())->pos();
      keyframe_lines.append(QLineF(QPointF(scene_bottom_left.x(), first_key_pos.y()), first_key_pos));

      // Draw lines between each keyframe
      for (int i=1;i<keys.size();i++) {
        NodeKeyframe* before = keys.at(i-1);
        NodeKeyframe* after = keys.at(i);

        KeyframeViewItem* before_item = item_map().value(before);
        KeyframeViewItem* after_item = item_map().value(after);

        if (before->type() == NodeKeyframe::kHold) {
          // Draw a hold keyframe (basically a right angle)
          keyframe_lines.append(QLineF(before_item->pos().x(),
                                       before_item->pos().y(),
                                       after_item->pos().x(),
                                       before_item->pos().y()));
          keyframe_lines.append(QLineF(after_item->pos().x(),
                                       before_item->pos().y(),
                                       after_item->pos().x(),
                                       after_item->pos().y()));
        } else if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {
          // Draw a cubic bezier

          // Cubic beziers have two control points, so we can just use both
          QPointF before_control_point = before_item->pos() + ScalePoint(before->bezier_control_out());
          QPointF after_control_point = after_item->pos() + ScalePoint(after->bezier_control_in());

          QPainterPath path;
          path.moveTo(before_item->pos());
          path.cubicTo(before_control_point, after_control_point, after_item->pos());
          painter->drawPath(path);

        } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
          // Draw a quadratic bezier

          // Quadratic beziers have a single control point, we just have to determine which it is
          QPointF key_anchor;
          QPointF control_point;

          if (before->type() == NodeKeyframe::kBezier) {
            key_anchor = before_item->pos();
            control_point = before->bezier_control_out();
          } else {
            key_anchor = after_item->pos();
            control_point = after->bezier_control_in();
          }

          // Scale control point
          control_point = key_anchor + ScalePoint(control_point);

          // Create the path from both keyframes
          QPainterPath path;
          path.moveTo(before_item->pos());
          path.quadTo(control_point, after_item->pos());
          painter->drawPath(path);

        } else {
          // Linear to linear
          keyframe_lines.append(QLineF(before_item->pos(), after_item->pos()));
        }
      }

      // Draw straight line leading from end keyframe
      QPointF last_key_pos = item_map().value(keys.last())->pos();
      keyframe_lines.append(QLineF(last_key_pos, QPointF(scene_top_right.x(), last_key_pos.y())));

      painter->drawLines(keyframe_lines);
    }
  }

  // Draw bezier control point lines
  if (!bezier_control_points_.isEmpty()) {
    painter->setPen(QPen(palette().text().color(), 1));

    QVector<QLineF> bezier_lines;
    foreach (BezierControlPointItem* item, bezier_control_points_) {
      // All BezierControlPointItems should be children of a KeyframeViewItem
      KeyframeViewItem* par = static_cast<KeyframeViewItem*>(item->parentItem());

      bezier_lines.append(QLineF(par->pos(), par->pos() + item->pos()));
    }
    painter->drawLines(bezier_lines);
  }
}

void CurveView::KeyframeAboutToBeRemoved(NodeKeyframe *key)
{
  disconnect(key, &NodeKeyframe::ValueChanged, this, &CurveView::KeyframeValueChanged);
  disconnect(key, &NodeKeyframe::TypeChanged, this, &CurveView::KeyframeTypeChanged);
}

void CurveView::ScaleChangedEvent(const double& scale)
{
  KeyframeViewBase::ScaleChangedEvent(scale);

  foreach (BezierControlPointItem* item, bezier_control_points_) {
    item->SetXScale(scale);
  }
}

void CurveView::VerticalScaleChangedEvent(double scale)
{
  Q_UNUSED(scale)

  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator iterator;

  for (iterator=item_map().begin();iterator!=item_map().end();iterator++) {
    SetItemYFromKeyframeValue(iterator.value()->key().get(), iterator.value());
  }
}

void CurveView::wheelEvent(QWheelEvent *event)
{
  if (!HandleZoomFromScroll(event)) {
    KeyframeViewBase::wheelEvent(event);
  }
}

void CurveView::ContextMenuEvent(Menu &m)
{
  m.addSeparator();

  // View settings
  QAction* zoom_fit_action = m.addAction(tr("Zoom to Fit"));
  connect(zoom_fit_action, &QAction::triggered, this, &CurveView::ZoomToFit);

  //QAction* reset_zoom_action = m.addAction(tr("Reset Zoom"));
}

QList<NodeKeyframe *> CurveView::GetKeyframesSortedByTime(int track)
{
  QList<NodeKeyframe *> sorted;

  for (auto it=item_map().cbegin();it!=item_map().cend();it++) {
    NodeKeyframe* key = it.key();

    if (key->track() != track) {
      continue;
    }

    bool inserted = false;

    for (int i=0;i<sorted.size();i++) {
      if (sorted.at(i)->time() > key->time()) {
        sorted.insert(i, key);
        inserted = true;
        break;
      }
    }

    if (!inserted) {
      sorted.append(key);
    }
  }

  return sorted;
}

qreal CurveView::GetItemYFromKeyframeValue(NodeKeyframe *key)
{
  return GetItemYFromKeyframeValue(key->value().toDouble());
}

qreal CurveView::GetItemYFromKeyframeValue(double value)
{
  return -value * GetYScale();
}

void CurveView::SetItemYFromKeyframeValue(NodeKeyframe *key, KeyframeViewItem *item)
{
  item->SetOverrideY(GetItemYFromKeyframeValue(key));
}

QPointF CurveView::ScalePoint(const QPointF &point)
{
  // Flips Y coordinate because curves are drawn bottom to top
  return QPointF(point.x() * GetScale(), - point.y() * GetYScale());
}

void CurveView::CreateBezierControlPoints(KeyframeViewItem* item)
{
  BezierControlPointItem* bezier_in_pt = new BezierControlPointItem(item->key(), NodeKeyframe::kInHandle, item);
  bezier_in_pt->SetXScale(GetScale());
  bezier_control_points_.append(bezier_in_pt);
  connect(bezier_in_pt, &QObject::destroyed, this, &CurveView::BezierControlPointDestroyed, Qt::DirectConnection);

  BezierControlPointItem* bezier_out_pt = new BezierControlPointItem(item->key(), NodeKeyframe::kOutHandle, item);
  bezier_out_pt->SetXScale(GetScale());
  bezier_control_points_.append(bezier_out_pt);
  connect(bezier_out_pt, &QObject::destroyed, this, &CurveView::BezierControlPointDestroyed, Qt::DirectConnection);
}

QColor CurveView::GetKeyframeColor(int track) const
{
  if (track_count_) {
    QColor c;
    c.setHsvF(static_cast<double>(track) / static_cast<double>(track_count_), 0.5, 1.0);
    return c;
  }

  return palette().text().color();
}

void CurveView::KeyframeValueChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  KeyframeViewItem* item = item_map().value(key);

  SetItemYFromKeyframeValue(key, item);
}

void CurveView::KeyframeTypeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  KeyframeViewItem* item = item_map().value(key);

  if (item->isSelected()) {
    item->setSelected(false);
    item->setSelected(true);
  }
}

void CurveView::SelectionChanged()
{
  // Clear current bezier handles
  foreach (BezierControlPointItem* item, bezier_control_points_) {
    delete item;
  }
  bezier_control_points_.clear();

  QList<QGraphicsItem*> selected = scene()->selectedItems();

  foreach (QGraphicsItem* item, selected) {
    KeyframeViewItem* this_item = static_cast<KeyframeViewItem*>(item);

    if (this_item->key()->type() == NodeKeyframe::kBezier) {
      CreateBezierControlPoints(this_item);
    }
  }
}

void CurveView::BezierControlPointDestroyed()
{
  BezierControlPointItem* item = static_cast<BezierControlPointItem*>(sender());
  bezier_control_points_.removeOne(item);
}

void CurveView::ZoomToFit()
{
  if (item_map().isEmpty()) {
    // Prevent scaling to DBL_MIN/DBL_MAX
    return;
  }

  QMap<NodeKeyframe *, KeyframeViewItem *>::const_iterator i;

  rational min_time = RATIONAL_MAX;
  rational max_time = RATIONAL_MIN;

  double min_val = DBL_MAX;
  double max_val = DBL_MIN;

  for (i=item_map().constBegin(); i!=item_map().constEnd(); i++) {
    rational transformed_time = GetAdjustedTime(i.key()->parent()->parentNode(),
                                                GetTimeTarget(),
                                                i.key()->time(),
                                                NodeParam::kOutput);

    min_time = qMin(transformed_time, min_time);
    max_time = qMax(transformed_time, max_time);

    min_val = qMin(i.key()->value().toDouble(), min_val);
    max_val = qMax(i.key()->value().toDouble(), max_val);
  }

  double time_range = max_time.toDouble() - min_time.toDouble();
  double new_x_scale = CalculateScaleFromDimensions(this->width(), time_range);
  double new_y_scale = CalculateScaleFromDimensions(this->height(), max_val - min_val);

  emit ScaleChanged(new_x_scale);
  SetYScale(new_y_scale);

  horizontalScrollBar()->setValue(TimeToScene(min_time) - CalculatePaddingFromDimensionScale(this->width()));
  verticalScrollBar()->setValue(GetItemYFromKeyframeValue(max_val) - CalculatePaddingFromDimensionScale(this->height()));
}

void CurveView::AddKeyframe(NodeKeyframePtr key)
{
  KeyframeViewItem* item = AddKeyframeInternal(key);
  SetItemYFromKeyframeValue(key.get(), item);
  item->SetOverrideBrush(GetKeyframeColor(key->track()));

  connect(key.get(), &NodeKeyframe::ValueChanged, this, &CurveView::KeyframeValueChanged);
  connect(key.get(), &NodeKeyframe::TypeChanged, this, &CurveView::KeyframeTypeChanged);
}

OLIVE_NAMESPACE_EXIT

#include "curveview.h"

#include <QtMath>

#include "common/qtversionabstraction.h"

CurveView::CurveView(QWidget *parent) :
  KeyframeViewBase(parent),
  y_scale_(1.0)
{
  setAlignment(Qt::AlignLeft | Qt::AlignBottom);
  setDragMode(RubberBandDrag);
  setViewportUpdateMode(FullViewportUpdate);

  text_padding_ = QFontMetricsWidth(fontMetrics(), QStringLiteral("i"));

  minimum_grid_space_ = QFontMetricsWidth(fontMetrics(), QStringLiteral("00000"));
}

void CurveView::Clear()
{
  KeyframeViewBase::Clear();

  foreach (QGraphicsLineItem* line, lines_) {
    delete line;
  }

  lines_.clear();
}

void CurveView::SetYScale(const double &y_scale)
{
  y_scale_ = y_scale;
  viewport()->update();
}

void CurveView::drawBackground(QPainter *painter, const QRectF &rect)
{
  if (timebase().isNull()) {
    return;
  }

  QVector<QLine> lines;

  double x_interval = timebase().flipped().toDouble();
  double y_interval = 100.0;

  int x_grid_interval, y_grid_interval;

  painter->setPen(QPen(palette().window().color(), 1));

  do {
    x_grid_interval = qRound(x_interval * scale_ * timebase_dbl());
    x_interval *= 2.0;
  } while (x_grid_interval < minimum_grid_space_);

  do {
    y_grid_interval = qRound(y_interval * y_scale_);
    y_interval *= 2.0;
  } while (y_grid_interval < minimum_grid_space_);

  int x_start = qCeil(rect.left() / x_grid_interval) * x_grid_interval;
  int y_start = qCeil(rect.top() / y_grid_interval) * y_grid_interval;

  QPointF scene_bottom_left = mapToScene(QPoint(0, qRound(rect.height())));
  QPointF scene_top_right = mapToScene(QPoint(qRound(rect.width()), 0));

  // Add vertical lines
  for (int i=x_start;i<rect.right();i+=x_grid_interval) {
    int value = qRound(static_cast<double>(i) / scale_ / timebase_dbl());
    painter->drawText(i + text_padding_, qRound(scene_bottom_left.y()) - text_padding_, QString::number(value));
    lines.append(QLine(i, qRound(rect.top()), i, qRound(rect.bottom())));
  }

  // Add horizontal lines
  for (int i=y_start;i<rect.bottom();i+=y_grid_interval) {
    int value = qRound(static_cast<double>(i) / y_scale_);
    painter->drawText(qRound(scene_bottom_left.x()) + text_padding_, i - text_padding_, QString::number(-value));
    lines.append(QLine(qRound(rect.left()), i, qRound(rect.right()), i));
  }

  // Draw grid
  painter->drawLines(lines);

  // Draw keyframe lines
  QList<NodeKeyframe*> keys = GetKeyframesSortedByTime();
  painter->setPen(QPen(palette().text().color(), 2));
  if (!keys.isEmpty()) {
    QVector<QLineF> keyframe_lines;

    // Draw straight line leading to first keyframe
    QPointF first_key_pos = item_map().value(keys.first())->center_pos();
    keyframe_lines.append(QLineF(QPointF(scene_bottom_left.x(), first_key_pos.y()), first_key_pos));

    // Draw lines between each keyframe
    for (int i=1;i<keys.size();i++) {
      NodeKeyframe* before = keys.at(i-1);
      NodeKeyframe* after = keys.at(i);

      KeyframeViewItem* before_item = item_map().value(before);
      KeyframeViewItem* after_item = item_map().value(after);

      if (before->type() == NodeKeyframe::kHold) {
        // Draw a hold keyframe (basically a right angle)
        keyframe_lines.append(QLineF(before_item->center_pos().x(),
                                     before_item->center_pos().y(),
                                     after_item->center_pos().x(),
                                     before_item->center_pos().y()));
        keyframe_lines.append(QLineF(after_item->center_pos().x(),
                                     before_item->center_pos().y(),
                                     after_item->center_pos().x(),
                                     after_item->center_pos().y()));
      } else if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {
        // Draw a cubic bezier
      } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
        // Draw a quadratic bezier
      } else {
        // Linear to linear
        keyframe_lines.append(QLineF(before_item->center_pos(), after_item->center_pos()));
      }
    }

    // Draw straight line leading from end keyframe
    QPointF last_key_pos = item_map().value(keys.last())->center_pos();
    keyframe_lines.append(QLineF(last_key_pos, QPointF(scene_top_right.x(), last_key_pos.y())));

    painter->drawLines(keyframe_lines);
  }
}

void CurveView::KeyframeAboutToBeRemoved(NodeKeyframe *key)
{
  disconnect(key, &NodeKeyframe::ValueChanged, this, &CurveView::KeyframeValueChanged);
}

QList<NodeKeyframe *> CurveView::GetKeyframesSortedByTime()
{
  QList<NodeKeyframe *> sorted;

  QMap<NodeKeyframe*, KeyframeViewItem*>::const_iterator iterator;

  for (iterator=item_map().begin();iterator!=item_map().end();iterator++) {
    NodeKeyframe* key = iterator.key();
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
  return -key->value().toDouble() * y_scale_;
}

void CurveView::SetItemYFromKeyframeValue(NodeKeyframe *key, KeyframeViewItem *item)
{
  item->SetOverrideY(GetItemYFromKeyframeValue(key));
}

void CurveView::KeyframeValueChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  KeyframeViewItem* item = item_map().value(key);

  SetItemYFromKeyframeValue(key, item);
}

void CurveView::AddKeyframe(NodeKeyframePtr key)
{
  KeyframeViewItem* item = AddKeyframeInternal(key);
  SetItemYFromKeyframeValue(key.get(), item);

  connect(key.get(), &NodeKeyframe::ValueChanged, this, &CurveView::KeyframeValueChanged);
}

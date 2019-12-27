#include "curveview.h"

#include <QtMath>

#include "common/qtversionabstraction.h"

CurveView::CurveView(QWidget *parent) :
  TimelineViewBase(parent)
{
  setAlignment(Qt::AlignLeft | Qt::AlignBottom);
  setDragMode(RubberBandDrag);
  setViewportUpdateMode(FullViewportUpdate);

  text_padding_ = QFontMetricsWidth(fontMetrics(), QStringLiteral("i"));
}

void CurveView::drawBackground(QPainter *painter, const QRectF &rect)
{
  QVector<QLine> lines;

  //painter->setPen(palette().window().color());

  int grid_interval = 100;

  int x_start = qCeil(rect.left() / grid_interval) * grid_interval;
  int y_start = qCeil(rect.top() / grid_interval) * grid_interval;

  QPoint viewport_edges(0, qRound(rect.height()));
  QPointF scene_edges = mapToScene(viewport_edges);
  //QRectF scene_edges = mapToScene(viewport()->geometry()).boundingRect();

  // Add vertical lines
  for (int i=x_start;i<rect.right();i+=grid_interval) {
    painter->drawText(i + text_padding_, qRound(scene_edges.y()) - text_padding_, QString::number(i));
    lines.append(QLine(i, qRound(rect.top()), i, qRound(rect.bottom())));
  }

  // Add horizontal lines
  for (int i=y_start;i<rect.bottom();i+=grid_interval) {
    painter->drawText(qRound(scene_edges.x()) + text_padding_, i - text_padding_, QString::number(-i));
    lines.append(QLine(qRound(rect.left()), i, qRound(rect.right()), i));
  }

  painter->drawLines(lines);
}

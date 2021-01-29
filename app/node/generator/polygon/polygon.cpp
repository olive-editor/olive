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

#include "polygon.h"

#include <QGuiApplication>
#include <QVector2D>

namespace olive {

PolygonGenerator::PolygonGenerator()
{
  points_input_ = new NodeInput(this, QStringLiteral("points_in"), NodeValue::kVec2, QVector2D(0, 0));
  points_input_->SetIsArray(true);

  color_input_ = new NodeInput(this, QStringLiteral("color_in"), NodeValue::kColor, QVariant::fromValue(Color(1.0, 1.0, 1.0)));

  // The Default Pentagon(tm)
  points_input_->ArrayResize(5);
  points_input_->SetStandardValueOnTrack(960, 0, 0);
  points_input_->SetStandardValueOnTrack(240, 1, 0);
  points_input_->SetStandardValueOnTrack(640, 0, 1);
  points_input_->SetStandardValueOnTrack(480, 1, 1);
  points_input_->SetStandardValueOnTrack(760, 0, 2);
  points_input_->SetStandardValueOnTrack(800, 1, 2);
  points_input_->SetStandardValueOnTrack(1100, 0, 3);
  points_input_->SetStandardValueOnTrack(800, 1, 3);
  points_input_->SetStandardValueOnTrack(1280, 0, 4);
  points_input_->SetStandardValueOnTrack(480, 1, 4);
}

Node *PolygonGenerator::copy() const
{
  return new PolygonGenerator();
}

QString PolygonGenerator::Name() const
{
  return tr("Polygon");
}

QString PolygonGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.polygon");
}

QVector<Node::CategoryID> PolygonGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString PolygonGenerator::Description() const
{
  return tr("Generate a 2D polygon of any amount of points.");
}

void PolygonGenerator::Retranslate()
{
  points_input_->set_name(tr("Points"));
  color_input_->set_name(tr("Color"));
}

ShaderCode PolygonGenerator::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/polygon.frag")));
}

NodeValueTable PolygonGenerator::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(points_input_, value);
  job.InsertValue(color_input_, value);
  job.InsertValue(QStringLiteral("resolution_in"), value[QStringLiteral("global")].GetWithMeta(NodeValue::kVec2, QStringLiteral("resolution")));
  job.SetAlphaChannelRequired(true);

  NodeValueTable table = value.Merge();
  table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  return table;
}

bool PolygonGenerator::HasGizmos() const
{
  return true;
}

/*void PolygonGenerator::DrawGizmos(NodeValueDatabase &db, QPainter *p) const
{
  Q_UNUSED(viewport)

  if (!points_input_->GetSize()) {
    return;
  }

  p->setPen(Qt::white);
  p->setBrush(Qt::white);

  QVector<QPointF> points = GetGizmoCoordinates(db, scale);
  QVector<QRectF> rects = GetGizmoRects(points);

  points.append(points.first());

  p->drawPolyline(points.constData(), points.size());
  p->drawRects(rects);
}

bool PolygonGenerator::GizmoPress(NodeValueDatabase &db, const QPointF &p, const QVector2D &scale, const QSize& viewport)
{
  Q_UNUSED(viewport)

  QVector<QPointF> points = GetGizmoCoordinates(db, scale);
  QVector<QRectF> rects = GetGizmoRects(points);

  for (int i=0;i<rects.size();i++) {
    const QRectF& r = rects.at(i);

    if (r.contains(p)) {
      gizmo_drag_ = points_input_->At(i);
      gizmo_drag_start_ = points.at(i);
      return true;
    }
  }

  return false;
}

void PolygonGenerator::GizmoMove(const QPointF &p, const QVector2D &scale, const rational& time)
{
  QVector2D new_pos = QVector2D(p) / scale;

  if (!gizmo_x_dragger_.IsStarted()) {
    gizmo_x_dragger_.Start(gizmo_drag_, time, 0);
  }

  if (!gizmo_y_dragger_.IsStarted()) {
    gizmo_y_dragger_.Start(gizmo_drag_, time, 1);
  }

  gizmo_x_dragger_.Drag(new_pos.x());
  gizmo_y_dragger_.Drag(new_pos.y());
}

void PolygonGenerator::GizmoRelease()
{
  gizmo_x_dragger_.End();
  gizmo_y_dragger_.End();
}*/

QVector<QPointF> PolygonGenerator::GetGizmoCoordinates(NodeValueDatabase &db, const QVector2D& scale) const
{
  // FIXME: Should Get() use a `kArray` type instead of a `kVec2` type?
  QVector<NodeValueTable> array_tbl = db[points_input_].Get(NodeValue::kVec2).value< QVector<NodeValueTable> >();
  QVector<QPointF> points(array_tbl.size());

  for (int i=0;i<points_input_->ArraySize();i++) {
    QVector2D v = array_tbl.at(i).Get(NodeValue::kVec2).value<QVector2D>();

    v *= scale;

    QPointF pt = v.toPointF();
    points[i] = pt;
  }

  return points;
}

QVector<QRectF> PolygonGenerator::GetGizmoRects(const QVector<QPointF> &points) const
{
  QVector<QRectF> rects(points.size());

  int rect_sz = QFontMetrics(qApp->font()).height() / 8;

  for (int i=0;i<points.size();i++) {
    const QPointF& p = points.at(i);

    rects[i] = QRectF(p - QPointF(rect_sz, rect_sz),
                      p + QPointF(rect_sz, rect_sz));
  }

  return rects;
}

}

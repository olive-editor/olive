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

#include "polygon.h"

#include <QGuiApplication>
#include <QVector2D>

OLIVE_NAMESPACE_ENTER

PolygonGenerator::PolygonGenerator()
{
  points_input_ = new NodeInputArray("points_in", NodeParam::kVec2);
  AddInput(points_input_);

  color_input_ = new NodeInput("color_in", NodeParam::kColor);
  AddInput(color_input_);

  // Default to "a color" that isn't
  color_input_->set_standard_value(1.0, 0);
  color_input_->set_standard_value(1.0, 1);
  color_input_->set_standard_value(1.0, 2);
  color_input_->set_standard_value(1.0, 3);

  // FIXME: Test code
  points_input_->SetSize(5);
  points_input_->At(0)->set_standard_value(960, 0);
  points_input_->At(0)->set_standard_value(240, 1);
  points_input_->At(1)->set_standard_value(640, 0);
  points_input_->At(1)->set_standard_value(480, 1);
  points_input_->At(2)->set_standard_value(760, 0);
  points_input_->At(2)->set_standard_value(800, 1);
  points_input_->At(3)->set_standard_value(1100, 0);
  points_input_->At(3)->set_standard_value(800, 1);
  points_input_->At(4)->set_standard_value(1280, 0);
  points_input_->At(4)->set_standard_value(480, 1);
  // End test
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

QList<Node::CategoryID> PolygonGenerator::Category() const
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

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/polygon.frag"), QString());
}

NodeValueTable PolygonGenerator::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(points_input_, value);
  job.InsertValue(color_input_, value);
  job.SetAlphaChannelRequired(true);

  NodeValueTable table = value.Merge();
  table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
  return table;
}

bool PolygonGenerator::HasGizmos() const
{
  return true;
}

void PolygonGenerator::DrawGizmos(NodeValueDatabase &db, QPainter *p, const QVector2D &scale, const QSize &viewport) const
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
}

QVector<QPointF> PolygonGenerator::GetGizmoCoordinates(NodeValueDatabase &db, const QVector2D& scale) const
{
  QVector<QPointF> points(points_input_->GetSize());

  for (int i=0;i<points_input_->GetSize();i++) {
    QVector2D v = db[points_input_->At(i)].Get(NodeParam::kVec2).value<QVector2D>();

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

OLIVE_NAMESPACE_EXIT

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

#include "matrix.h"

#include <QGuiApplication>
#include <QMatrix4x4>
#include <QVector2D>

#include "common/range.h"

OLIVE_NAMESPACE_ENTER

MatrixGenerator::MatrixGenerator()
{
  position_input_ = new NodeInput("pos_in", NodeParam::kVec2, QVector2D());
  AddInput(position_input_);

  rotation_input_ = new NodeInput("rot_in", NodeParam::kFloat, 0.0f);
  AddInput(rotation_input_);

  scale_input_ = new NodeInput("scale_in", NodeParam::kVec2, QVector2D(1.0f, 1.0f));
  scale_input_->set_property("min", QVector2D(0, 0));
  scale_input_->set_property("view", "percent");
  scale_input_->set_property("disabley", true);
  AddInput(scale_input_);

  uniform_scale_input_ = new NodeInput("uniform_scale_in", NodeParam::kBoolean, true);
  uniform_scale_input_->set_is_keyframable(false);
  uniform_scale_input_->SetConnectable(false);
  connect(uniform_scale_input_, &NodeInput::ValueChanged, this, &MatrixGenerator::UniformScaleChanged);
  AddInput(uniform_scale_input_);

  anchor_input_ = new NodeInput("anchor_in", NodeParam::kVec2, QVector2D());
  AddInput(anchor_input_);
}

Node *MatrixGenerator::copy() const
{
  return new MatrixGenerator();
}

QString MatrixGenerator::Name() const
{
  return tr("Orthographic Matrix");
}

QString MatrixGenerator::ShortName() const
{
  return tr("Ortho");
}

QString MatrixGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.transform");
}

QList<Node::CategoryID> MatrixGenerator::Category() const
{
  return {kCategoryGenerator, kCategoryMath};
}

QString MatrixGenerator::Description() const
{
  return tr("Generate an orthographic matrix using position, rotation, and scale.");
}

void MatrixGenerator::Retranslate()
{
  position_input_->set_name(tr("Position"));
  rotation_input_->set_name(tr("Rotation"));
  scale_input_->set_name(tr("Scale"));
  uniform_scale_input_->set_name(tr("Uniform Scale"));
  anchor_input_->set_name(tr("Anchor Point"));
}

NodeValueTable MatrixGenerator::Value(NodeValueDatabase &value) const
{
  // Push matrix output
  QMatrix4x4 mat = GenerateMatrix(value);
  NodeValueTable output = value.Merge();
  output.Push(NodeParam::kMatrix, mat);
  return output;
}

bool MatrixGenerator::GizmoPress(const NodeValueDatabase &db, const QPointF &p, const QVector2D &scale, const QSize &viewport)
{
  GizmoSharedData gizmo_data(viewport, scale);

  QPointF anchor_pt = GetGizmoAnchorPoint(db, gizmo_data);
  int anchor_radius = GetGizmoAnchorPointRadius();

  if (InRange(p.x(), anchor_pt.x(), anchor_radius * 2.0)
      && InRange(p.y(), anchor_pt.y(), anchor_radius * 2.0)) {
    gizmo_drag_ = anchor_input_;
    gizmo_start2_ = db[position_input_].Get(NodeParam::kVec2).value<QVector2D>();
  } else {
    gizmo_drag_ = position_input_;
  }

  gizmo_start_ = db[gizmo_drag_].Get(NodeParam::kVec2).value<QVector2D>();
  gizmo_mouse_start_ = p;

  return true;
}

void MatrixGenerator::GizmoMove(const QPointF &p, const QVector2D &scale, const rational &time)
{
  QVector2D movement = 2.0 * QVector2D(p - gizmo_mouse_start_) / scale;
  QVector2D new_pos = gizmo_start_ + movement;

  if (!gizmo_x_dragger_.IsStarted()) {
    gizmo_x_dragger_.Start(gizmo_drag_, time, 0);
  }

  if (!gizmo_y_dragger_.IsStarted()) {
    gizmo_y_dragger_.Start(gizmo_drag_, time, 1);
  }

  gizmo_x_dragger_.Drag(new_pos.x());
  gizmo_y_dragger_.Drag(new_pos.y());

  if (gizmo_drag_ == anchor_input_) {
    // If we're dragging the anchor, counter the position at the same time
    QVector2D new_pos2 = gizmo_start2_ + movement;

    if (!gizmo_x2_dragger_.IsStarted()) {
      gizmo_x2_dragger_.Start(position_input_, time, 0);
    }

    if (!gizmo_y2_dragger_.IsStarted()) {
      gizmo_y2_dragger_.Start(position_input_, time, 1);
    }

    gizmo_x2_dragger_.Drag(new_pos2.x());
    gizmo_y2_dragger_.Drag(new_pos2.y());
  }
}

void MatrixGenerator::GizmoRelease()
{
  gizmo_x_dragger_.End();
  gizmo_y_dragger_.End();
  gizmo_x2_dragger_.End();
  gizmo_y2_dragger_.End();
}

bool MatrixGenerator::HasGizmos() const
{
  return true;
}

void MatrixGenerator::DrawGizmos(const NodeValueDatabase &db, QPainter *p, const QVector2D &scale, const QSize& viewport) const
{
  p->setPen(Qt::white);

  GizmoSharedData gizmo_data(viewport, scale);

  {
    // Fold values into a matrix
    QMatrix4x4 matrix;
    matrix.scale(gizmo_data.half_scale);
    matrix *= GenerateMatrix(db, false);
    matrix.scale(gizmo_data.inverted_half_scale);

    // Create rect and transform it
    QVector<QPointF> points = {QPointF(-100, -100),
                               QPointF(100, -100),
                               QPointF(100, 100),
                               QPointF(-100, 100),
                               QPointF(-100, -100)};
    QPolygonF poly(points);
    poly = matrix.toTransform().map(poly);
    poly.translate(gizmo_data.half_viewport);

    // Draw square
    p->drawPolyline(poly);
  }

  {
    // Draw anchor point marker (with no anchor point translation)
    QPointF pt = GetGizmoAnchorPoint(db, gizmo_data);

    // Draw anchor point
    int anchor_pt_radius = GetGizmoAnchorPointRadius();

    p->drawEllipse(pt, anchor_pt_radius, anchor_pt_radius);

    p->drawLines({QLineF(pt.x() - anchor_pt_radius, pt.y(),
                  pt.x() + anchor_pt_radius, pt.y()),
                  QLineF(pt.x(), pt.y() - anchor_pt_radius,
                  pt.x(), pt.y() + anchor_pt_radius)});
  }
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(NodeValueDatabase &value) const
{
  return GenerateMatrix(value[position_input_].Take(NodeParam::kVec2).value<QVector2D>(),
                        value[rotation_input_].Take(NodeParam::kFloat).toFloat(),
                        value[scale_input_].Take(NodeParam::kVec2).value<QVector2D>(),
                        value[uniform_scale_input_].Take(NodeParam::kBoolean).toBool(),
                        value[anchor_input_].Take(NodeParam::kVec2).value<QVector2D>());
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(const NodeValueDatabase &value, bool ignore_anchor) const
{
  QVector2D anchor;

  if (!ignore_anchor) {
    anchor = value[anchor_input_].Get(NodeParam::kVec2).value<QVector2D>();
  }

  return GenerateMatrix(value[position_input_].Get(NodeParam::kVec2).value<QVector2D>(),
                        value[rotation_input_].Get(NodeParam::kFloat).toFloat(),
                        value[scale_input_].Get(NodeParam::kVec2).value<QVector2D>(),
                        value[uniform_scale_input_].Get(NodeParam::kBoolean).toBool(),
                        anchor);
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(const QVector2D& pos,
                                           const float& rot,
                                           const QVector2D& scale,
                                           bool uniform_scale,
                                           const QVector2D& anchor)
{
  QMatrix4x4 mat;

  // Position
  mat.translate(pos);

  // Rotation
  mat.rotate(rot, 0, 0, 1);

  // Scale
  if (uniform_scale) {
    QVector2D uniformed(scale.x(), scale.x());
    mat.scale(uniformed);
  } else {
    mat.scale(scale);
  }

  // Anchor Point
  mat.translate(-anchor);

  return mat;
}

QPointF MatrixGenerator::GetGizmoAnchorPoint(const NodeValueDatabase &db,
                                            const GizmoSharedData& gizmo_data) const
{
  QMatrix4x4 matrix;
  matrix.scale(gizmo_data.half_scale);
  matrix *= GenerateMatrix(db, true);
  matrix.scale(gizmo_data.inverted_half_scale);

  QPointF pt = matrix.toTransform().map(QPointF());
  pt += gizmo_data.half_viewport;

  return pt;
}

int MatrixGenerator::GetGizmoAnchorPointRadius()
{
  return QFontMetrics(qApp->font()).height() / 2;
}

void MatrixGenerator::UniformScaleChanged()
{
  scale_input_->set_property("disabley", uniform_scale_input_->get_standard_value().toBool());
}

MatrixGenerator::GizmoSharedData::GizmoSharedData(const QSize &viewport, const QVector2D &scale)
{
  half_viewport = QPointF(viewport.width() / 2, viewport.height() / 2);
  half_scale = scale * 0.5;
  inverted_half_scale = QVector2D(1.0f / half_scale.x(), 1.0f / half_scale.y());
}

OLIVE_NAMESPACE_EXIT

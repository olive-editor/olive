/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "shapenodebase.h"

#include <QtMath>
#include <QVector2D>

#include "common/util.h"
#include "core.h"
#include "node/nodeundo.h"

namespace olive {

#define super GeneratorWithMerge

const QString ShapeNodeBase::kPositionInput = QStringLiteral("pos_in");
const QString ShapeNodeBase::kSizeInput = QStringLiteral("size_in");
const QString ShapeNodeBase::kColorInput = QStringLiteral("color_in");

ShapeNodeBase::ShapeNodeBase(bool create_color_input)
{
  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));
  AddInput(kSizeInput, NodeValue::kVec2, QVector2D(100, 100));
  SetInputProperty(kSizeInput, QStringLiteral("min"), QVector2D(0, 0));

  if (create_color_input) {
    AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0, 0.0, 0.0, 1.0)));
  }

  // Initiate gizmos
  QVector<NodeKeyframeTrackReference> pos_n_sz = {
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1),
    NodeKeyframeTrackReference(NodeInput(this, kSizeInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kSizeInput), 1)
  };
  poly_gizmo_ = AddDraggableGizmo<PolygonGizmo>({
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1),
  });
  for (int i=0; i<kGizmoScaleCount; i++) {
    point_gizmo_[i] = AddDraggableGizmo<PointGizmo>(pos_n_sz, PointGizmo::kAbsolute);
  }
}

void ShapeNodeBase::Retranslate()
{
  super::Retranslate();

  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kSizeInput, tr("Size"));

  if (HasInputWithID(kColorInput)) {
    SetInputName(kColorInput, tr("Color"));
  }
}

void ShapeNodeBase::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  // Use offsets to make the appearance of values that start in the top left, even though we
  // really anchor around the center
  QVector2D center_pt = globals.square_resolution() * 0.5;
  SetInputProperty(kPositionInput, QStringLiteral("offset"), center_pt);

  QVector2D pos = row[kPositionInput].toVec2();
  QVector2D sz = row[kSizeInput].toVec2();
  QVector2D half_sz = sz * 0.5;

  double left_pt = pos.x() + center_pt.x() - half_sz.x();
  double top_pt = pos.y() + center_pt.y() - half_sz.y();
  double right_pt = left_pt + sz.x();
  double bottom_pt = top_pt + sz.y();
  double center_x_pt = mid(left_pt, right_pt);
  double center_y_pt = mid(top_pt, bottom_pt);

  point_gizmo_[kGizmoScaleTopLeft]->SetPoint(QPointF(left_pt, top_pt));
  point_gizmo_[kGizmoScaleTopCenter]->SetPoint(QPointF(center_x_pt, top_pt));
  point_gizmo_[kGizmoScaleTopRight]->SetPoint(QPointF(right_pt, top_pt));
  point_gizmo_[kGizmoScaleBottomLeft]->SetPoint(QPointF(left_pt, bottom_pt));
  point_gizmo_[kGizmoScaleBottomCenter]->SetPoint(QPointF(center_x_pt, bottom_pt));
  point_gizmo_[kGizmoScaleBottomRight]->SetPoint(QPointF(right_pt, bottom_pt));
  point_gizmo_[kGizmoScaleCenterLeft]->SetPoint(QPointF(left_pt, center_y_pt));
  point_gizmo_[kGizmoScaleCenterRight]->SetPoint(QPointF(right_pt, center_y_pt));

  poly_gizmo_->SetPolygon(QRectF(left_pt, top_pt, right_pt - left_pt, bottom_pt - top_pt));
}

void ShapeNodeBase::SetRect(QRectF rect, const VideoParams &sequence_res, MultiUndoCommand *command)
{
  // Normalize around center of sequence
  rect.translate(-sequence_res.width()*0.5, -sequence_res.height()*0.5);
  rect.translate(rect.width()*0.5, rect.height()*0.5);

  NodeInput pos(this, ShapeNodeBase::kPositionInput);
  NodeInput sz(this, ShapeNodeBase::kSizeInput);

  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(sz, 0), rect.width()));
  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(sz, 1), rect.height()));
  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(pos, 0), rect.x()));
  command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(pos, 1), rect.y()));
}

void ShapeNodeBase::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  NodeInputDragger &x_drag = gizmo->GetDraggers()[0];
  NodeInputDragger &y_drag = gizmo->GetDraggers()[1];

  if (gizmo == poly_gizmo_) {
    x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
    y_drag.Drag(y_drag.GetStartValue().toDouble() + y);
  } else {
    bool from_center = modifiers & Qt::AltModifier;
    bool keep_ratio = modifiers & Qt::ShiftModifier;

    NodeInputDragger &w_drag = gizmo->GetDraggers()[2];
    NodeInputDragger &h_drag = gizmo->GetDraggers()[3];

    QVector2D gizmo_sz_start(w_drag.GetStartValue().toDouble(), h_drag.GetStartValue().toDouble());
    QVector2D gizmo_pos_start(x_drag.GetStartValue().toDouble(), y_drag.GetStartValue().toDouble());
    QVector2D gizmo_half_res = gizmo->GetGlobals().square_resolution()/2;
    QVector2D adjusted_pt(x, y);
    QVector2D new_size;
    QVector2D new_pos;
    QVector2D anchor;
    static const int kXYCount = 2;
    bool negative[kXYCount] = {false};

    double original_ratio;
    if (keep_ratio) {
      original_ratio = w_drag.GetStartValue().toDouble() / h_drag.GetStartValue().toDouble();
    }

    // Calculate new size
    if (from_center) {
      // Calculate new size by using distance from center and doubling it
      new_size = (adjusted_pt - gizmo_half_res - gizmo_pos_start) * 2;

      if (IsGizmoTop(gizmo)) {
        new_size.setY(-new_size.y());
      }

      if (IsGizmoLeft(gizmo)) {
        new_size.setX(-new_size.x());
      }
    } else {
      // Calculate new size by using distance from "anchor" - i.e. the opposite point of the shape
      // from the gizmo being dragged
      adjusted_pt -= gizmo_half_res;

      anchor = GenerateGizmoAnchor(gizmo_pos_start, gizmo_sz_start, gizmo, &adjusted_pt) + gizmo_half_res;

      adjusted_pt += gizmo_half_res;

      // Calculate size and position
      new_size = adjusted_pt - anchor;

      // Abs size so neither coord is negative
      for (int i=0; i<kXYCount; i++) {
        if (new_size[i] < 0) {
          negative[i] = true;
          new_size[i] = -new_size[i];
        }
      }
    }

    // Restrict sizes by constraints
    if (IsGizmoVerticalCenter(gizmo)) {
      if (keep_ratio) {
        // Calculate width from new height
        new_size.setX(new_size.y() * original_ratio);
      } else {
        // Constrain to original width
        new_size.setX(gizmo_sz_start.x());
      }
    }

    if (IsGizmoHorizontalCenter(gizmo)) {
      if (keep_ratio) {
        // Calculate height from new width
        new_size.setY(new_size.x() / original_ratio);
      } else {
        // Constrain to original height
        new_size.setY(gizmo_sz_start.y());
      }
    }

    if (IsGizmoCorner(gizmo)) {
      if (keep_ratio) {
        float hypot = std::hypot(new_size.x(), new_size.y());

        float original_angle = std::atan2(gizmo_sz_start.x(), gizmo_sz_start.y());

        // Calculate new size based on original angle and hypotenuse
        new_size.setX(std::sin(original_angle) * hypot);
        new_size.setY(std::cos(original_angle) * hypot);
      }
    }

    // Calculate position
    if (from_center) {
      new_pos = gizmo_pos_start;
    } else {
      QVector2D using_size = new_size;

      // Un-abs size
      for (int i=0; i<kXYCount; i++) {
        if (negative[i]) {
          using_size[i] = -using_size[i];
        }
      }

      // I'm pretty sure there's an algorithmic way of doing this, but I'm tired and this works
      if (IsGizmoHorizontalCenter(gizmo)) {
        using_size.setY(0);
      }

      if (IsGizmoVerticalCenter(gizmo)) {
        using_size.setX(0);
      }

      new_pos = GenerateGizmoAnchor(gizmo_pos_start, gizmo_sz_start, gizmo) + using_size / 2;
    }

    x_drag.Drag(new_pos.x());
    y_drag.Drag(new_pos.y());
    w_drag.Drag(new_size.x());
    h_drag.Drag(new_size.y());
  }
}

QVector2D ShapeNodeBase::GenerateGizmoAnchor(const QVector2D &pos, const QVector2D &size, NodeGizmo *gizmo, QVector2D *pt) const
{
  QVector2D anchor = pos;
  QVector2D half_sz = size/2;

  if (IsGizmoLeft(gizmo)) {
    anchor.setX(anchor.x() + half_sz.x());
    if (pt && pt->x() > anchor.x()) {
      pt->setX(anchor.x());
    }
  }

  if (IsGizmoRight(gizmo)) {
    anchor.setX(anchor.x() - half_sz.x());
    if (pt && pt->x() < anchor.x()) {
      pt->setX(anchor.x());
    }
  }

  if (IsGizmoTop(gizmo)) {
    anchor.setY(anchor.y() + half_sz.y());
    if (pt && pt->y() > anchor.y()) {
      pt->setY(anchor.y());
    }
  }

  if (IsGizmoBottom(gizmo)) {
    anchor.setY(anchor.y() - half_sz.y());
    if (pt && pt->y() < anchor.y()) {
      pt->setY(anchor.y());
    }
  }

  return anchor;
}

bool ShapeNodeBase::IsGizmoTop(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleTopCenter] || g == point_gizmo_[kGizmoScaleTopLeft] || g == point_gizmo_[kGizmoScaleTopRight];
}

bool ShapeNodeBase::IsGizmoBottom(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleBottomCenter] || g == point_gizmo_[kGizmoScaleBottomLeft] || g == point_gizmo_[kGizmoScaleBottomRight];
}

bool ShapeNodeBase::IsGizmoLeft(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleTopLeft] || g == point_gizmo_[kGizmoScaleCenterLeft] || g == point_gizmo_[kGizmoScaleBottomLeft];
}

bool ShapeNodeBase::IsGizmoRight(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleTopRight] || g == point_gizmo_[kGizmoScaleCenterRight] || g == point_gizmo_[kGizmoScaleBottomRight];
}

bool ShapeNodeBase::IsGizmoHorizontalCenter(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleCenterLeft] || g == point_gizmo_[kGizmoScaleCenterRight];
}

bool ShapeNodeBase::IsGizmoVerticalCenter(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleTopCenter] || g == point_gizmo_[kGizmoScaleBottomCenter];
}

bool ShapeNodeBase::IsGizmoCorner(NodeGizmo *g) const
{
  return g == point_gizmo_[kGizmoScaleTopLeft] || g == point_gizmo_[kGizmoScaleTopRight]
      || g == point_gizmo_[kGizmoScaleBottomRight] || g == point_gizmo_[kGizmoScaleBottomLeft];
}

}

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

#include "shapenodebase.h"

#include <QtMath>
#include <QVector2D>

#include "common/lerp.h"
#include "core.h"

namespace olive {

#define super Node

QString ShapeNodeBase::kPositionInput = QStringLiteral("pos_in");
QString ShapeNodeBase::kSizeInput = QStringLiteral("size_in");
QString ShapeNodeBase::kColorInput = QStringLiteral("color_in");

ShapeNodeBase::ShapeNodeBase()
{
  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));
  AddInput(kSizeInput, NodeValue::kVec2, QVector2D(100, 100));
  SetInputProperty(kSizeInput, QStringLiteral("min"), QVector2D(0, 0));
  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0, 0.0, 0.0, 1.0)));
}

void ShapeNodeBase::Retranslate()
{
  super::Retranslate();

  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kSizeInput, tr("Size"));
  SetInputName(kColorInput, tr("Color"));
}

void ShapeNodeBase::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  // Use offsets to make the appearance of values that start in the top left, even though we
  // really anchor around the center
  QVector2D center_pt = globals.resolution() * 0.5;
  SetInputProperty(kPositionInput, QStringLiteral("offset"), center_pt);

  const double handle_radius = GetGizmoHandleRadius(p->transform());

  p->setPen(QPen(Qt::white, 0));

  QVector2D pos = row[kPositionInput].data().value<QVector2D>();
  QVector2D sz = row[kSizeInput].data().value<QVector2D>();
  QVector2D half_sz = sz * 0.5;

  double left_pt = pos.x() + center_pt.x() - half_sz.x();
  double top_pt = pos.y() + center_pt.y() - half_sz.y();
  double right_pt = left_pt + sz.x();
  double bottom_pt = top_pt + sz.y();
  double center_x_pt = lerp(left_pt, right_pt, 0.5);
  double center_y_pt = lerp(top_pt, bottom_pt, 0.5);

  gizmo_whole_rect_ = QRectF(left_pt, top_pt, right_pt - left_pt, bottom_pt - top_pt);
  p->drawRect(gizmo_whole_rect_);

  gizmo_resize_handle_[kGizmoScaleTopLeft] = CreateGizmoHandleRect(QPointF(left_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleTopCenter] = CreateGizmoHandleRect(QPointF(center_x_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleTopRight] = CreateGizmoHandleRect(QPointF(right_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomLeft] = CreateGizmoHandleRect(QPointF(left_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomCenter] = CreateGizmoHandleRect(QPointF(center_x_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomRight] = CreateGizmoHandleRect(QPointF(right_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleCenterLeft] = CreateGizmoHandleRect(QPointF(left_pt, center_y_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleCenterRight] = CreateGizmoHandleRect(QPointF(right_pt, center_y_pt), handle_radius);

  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_resize_handle_, kGizmoScaleCount);
}

bool ShapeNodeBase::GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p)
{
  gizmo_drag_ = -1;

  // See if any of our resize handles have the pointer
  for (int i=0; i<kGizmoScaleCount; i++) {
    if (gizmo_resize_handle_[i].contains(p)) {
      gizmo_drag_ = i;
      break;
    }
  }

  // See if the rect has the pointer
  if (gizmo_drag_ == -1 && gizmo_whole_rect_.contains(p)) {
    gizmo_drag_ = kGizmoWholeRect;
  }

  if (gizmo_drag_ >= 0) {
    gizmo_pos_start_ = row[kPositionInput].data().value<QVector2D>();
    gizmo_sz_start_ = row[kSizeInput].data().value<QVector2D>();
    gizmo_drag_start_ = p;
    gizmo_half_res_ = globals.resolution()/2;

    // Get aspect ratio (avoiding potential zero)
    if (qFuzzyIsNull(gizmo_sz_start_.y())) {
      gizmo_aspect_ratio_ = 0;
    } else {
      gizmo_aspect_ratio_ = gizmo_sz_start_.x() / gizmo_sz_start_.y();
    }
    return true;
  }

  return false;
}

enum {
  kGDPosX,
  kGDPosY,
  kGDSzX,
  kGDSzY,
  kGDCount
};

QVector2D ShapeNodeBase::GenerateGizmoAnchor(const QVector2D &pos, const QVector2D &size, int drag, QVector2D *pt = nullptr)
{
  QVector2D anchor = pos;
  QVector2D half_sz = size/2;

  if (drag == kGizmoScaleTopLeft || drag == kGizmoScaleCenterLeft || drag == kGizmoScaleBottomLeft) {
    anchor.setX(anchor.x() + half_sz.x());
    if (pt && pt->x() > anchor.x()) {
      pt->setX(anchor.x());
    }
  }

  if (drag == kGizmoScaleTopRight || drag == kGizmoScaleCenterRight || drag == kGizmoScaleBottomRight) {
    anchor.setX(anchor.x() - half_sz.x());
    if (pt && pt->x() < anchor.x()) {
      pt->setX(anchor.x());
    }
  }

  if (drag == kGizmoScaleTopLeft || drag == kGizmoScaleTopCenter || drag == kGizmoScaleTopRight) {
    anchor.setY(anchor.y() + half_sz.y());
    if (pt && pt->y() > anchor.y()) {
      pt->setY(anchor.y());
    }
  }

  if (drag == kGizmoScaleBottomLeft || drag == kGizmoScaleBottomCenter || drag == kGizmoScaleBottomRight) {
    anchor.setY(anchor.y() - half_sz.y());
    if (pt && pt->y() < anchor.y()) {
      pt->setY(anchor.y());
    }
  }

  return anchor;
}

void ShapeNodeBase::GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers)
{
  if (gizmo_dragger_.isEmpty()) {
    gizmo_dragger_.resize(kGDCount);
    gizmo_dragger_[kGDPosX].Start(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0), time);
    gizmo_dragger_[kGDPosY].Start(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1), time);
    gizmo_dragger_[kGDSzX].Start(NodeKeyframeTrackReference(NodeInput(this, kSizeInput), 0), time);
    gizmo_dragger_[kGDSzY].Start(NodeKeyframeTrackReference(NodeInput(this, kSizeInput), 1), time);
  }

  bool from_center = modifiers & Qt::AltModifier;
  bool keep_ratio = modifiers & Qt::ShiftModifier;

  QVector2D adjusted_pt(p.x(), p.y());
  QVector2D diff(adjusted_pt.x() - gizmo_drag_start_.x(), adjusted_pt.y() - gizmo_drag_start_.y());

  if (gizmo_drag_ == kGizmoWholeRect) {
    // Simply move position by difference
    gizmo_dragger_[kGDPosX].Drag(gizmo_pos_start_.x() + diff.x());
    gizmo_dragger_[kGDPosY].Drag(gizmo_pos_start_.y() + diff.y());
  } else {

    QVector2D new_size;
    QVector2D new_pos;
    QVector2D anchor;
    static const int kXYCount = 2;
    bool negative[kXYCount] = {false};

    // Calculate new size
    if (from_center) {
      // Calculate new size by using distance from center and doubling it
      new_size = (adjusted_pt - gizmo_half_res_ - gizmo_pos_start_) * 2;

      if (gizmo_drag_ == kGizmoScaleTopCenter || gizmo_drag_ == kGizmoScaleTopLeft || gizmo_drag_ == kGizmoScaleTopRight) {
        new_size.setY(-new_size.y());
      }

      if (gizmo_drag_ == kGizmoScaleTopLeft || gizmo_drag_ == kGizmoScaleCenterLeft || gizmo_drag_ == kGizmoScaleBottomLeft) {
        new_size.setX(-new_size.x());
      }
    } else {
      // Calculate new size by using distance from "anchor" - i.e. the opposite point of the shape
      // from the gizmo being dragged
      adjusted_pt -= gizmo_half_res_;

      anchor = GenerateGizmoAnchor(gizmo_pos_start_, gizmo_sz_start_, gizmo_drag_, &adjusted_pt) + gizmo_half_res_;

      adjusted_pt += gizmo_half_res_;

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
    if (gizmo_drag_ == kGizmoScaleTopCenter || gizmo_drag_ == kGizmoScaleBottomCenter) {
      if (keep_ratio) {
        // Calculate width from new height
        new_size.setX(new_size.y() * gizmo_aspect_ratio_);
      } else {
        // Constrain to original width
        new_size.setX(gizmo_sz_start_.x());
      }
    }

    if (gizmo_drag_ == kGizmoScaleCenterLeft || gizmo_drag_ == kGizmoScaleCenterRight) {
      if (keep_ratio) {
        // Calculate height from new width
        new_size.setY(new_size.x() / gizmo_aspect_ratio_);
      } else {
        // Constrain to original height
        new_size.setY(gizmo_sz_start_.y());
      }
    }

    if (gizmo_drag_ == kGizmoScaleTopLeft || gizmo_drag_ == kGizmoScaleTopRight
        || gizmo_drag_ == kGizmoScaleBottomRight || gizmo_drag_ == kGizmoScaleBottomLeft) {
      if (keep_ratio) {
        float hypot = std::hypot(new_size.x(), new_size.y());

        float original_angle = std::atan2(gizmo_sz_start_.x(), gizmo_sz_start_.y());

        // Calculate new size based on original angle and hypotenuse
        new_size.setX(std::sin(original_angle) * hypot);
        new_size.setY(std::cos(original_angle) * hypot);
      }
    }

    // Calculate position
    if (from_center) {
      new_pos = gizmo_pos_start_;
    } else {
      QVector2D using_size = new_size;

      // Un-abs size
      for (int i=0; i<kXYCount; i++) {
        if (negative[i]) {
          using_size[i] = -using_size[i];
        }
      }

      // I'm pretty sure there's an algorithmic way of doing this, but I'm tired and this works
      if (gizmo_drag_ == kGizmoScaleCenterLeft || gizmo_drag_ == kGizmoScaleCenterRight) {
        using_size.setY(0);
      }

      if (gizmo_drag_ == kGizmoScaleTopCenter || gizmo_drag_ == kGizmoScaleBottomCenter) {
        using_size.setX(0);
      }

      new_pos = GenerateGizmoAnchor(gizmo_pos_start_, gizmo_sz_start_, gizmo_drag_) + using_size / 2;
    }

    gizmo_dragger_[kGDPosX].Drag(new_pos.x());
    gizmo_dragger_[kGDPosY].Drag(new_pos.y());
    gizmo_dragger_[kGDSzX].Drag(new_size.x());
    gizmo_dragger_[kGDSzY].Drag(new_size.y());

  }
}

void ShapeNodeBase::GizmoRelease(MultiUndoCommand *command)
{
  for (NodeInputDragger& i : gizmo_dragger_) {
    i.End(command);
  }
  gizmo_dragger_.clear();
}

}

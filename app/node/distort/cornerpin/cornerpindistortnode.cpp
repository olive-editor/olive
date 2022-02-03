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

#include "cornerpindistortnode.h"

#include "common/lerp.h"
#include "core.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString CornerPinDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString CornerPinDistortNode::kTopLeftInput = QStringLiteral("top_left_in");
const QString CornerPinDistortNode::kTopRightInput = QStringLiteral("top_right_in");
const QString CornerPinDistortNode::kBottomRightInput = QStringLiteral("bottom_right_in");
const QString CornerPinDistortNode::kBottomLeftInput = QStringLiteral("bottom_left_in");
const QString CornerPinDistortNode::kPerspectiveInput = QStringLiteral("perspective_in");

CornerPinDistortNode::CornerPinDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  AddInput(kPerspectiveInput, NodeValue::kBoolean, true);
  AddInput(kTopLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kTopRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
}

void CornerPinDistortNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kPerspectiveInput, tr("Perspective"));
  SetInputName(kTopLeftInput, tr("Top Left"));
  SetInputName(kTopRightInput, tr("Top Right"));
  SetInputName(kBottomRightInput, tr("Bottom Right"));
  SetInputName(kBottomLeftInput, tr("Bottom Left"));
}

void CornerPinDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  // Convert slider values to their pixel values and then convert to clip space (-1.0 ... 1.0) for overriding the
  // vertex coordinates.
  const QVector2D &resolution = globals.resolution();
  QVector2D half_resolution = resolution * 0.5;
  QVector2D top_left = QVector2D(ValueToPixel(0, value, resolution)) / half_resolution - QVector2D(1.0, 1.0);
  QVector2D top_right = QVector2D(ValueToPixel(1, value, resolution)) / half_resolution - QVector2D(1.0, 1.0);
  QVector2D bottom_right = QVector2D(ValueToPixel(2, value, resolution)) / half_resolution - QVector2D(1.0, 1.0);
  QVector2D bottom_left = QVector2D(ValueToPixel(3, value, resolution)) / half_resolution - QVector2D(1.0, 1.0);

  // Override default vertex coordinates.
  QVector<float> adjusted_vertices = {top_left.x(), top_left.y(), 0.0f,
                                  top_right.x(), top_right.y(), 0.0f,
                                  bottom_right.x(),  bottom_right.y(), 0.0f,

                                  top_left.x(), top_left.y(), 0.0f,
                                  bottom_left.x(),  bottom_left.y(), 0.0f,
                                  bottom_right.x(),  bottom_right.y(), 0.0f};
  job.SetVertexCoordinates(adjusted_vertices);

  // If no texture do nothing
  if (!job.GetValue(kTextureInput).data().isNull()) {
    // In the special case that all sliders are in their default position just
    // push the texture.
    if (!(job.GetValue(kTopLeftInput).data().value<QVector2D>().isNull()
        && job.GetValue(kTopRightInput).data().value<QVector2D>().isNull() &&
        job.GetValue(kBottomRightInput).data().value<QVector2D>().isNull() &&
        job.GetValue(kBottomLeftInput).data().value<QVector2D>().isNull())) {
      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table->Push(NodeValue::kTexture, job.GetValue(kTextureInput).data(), this);
    }
  }
}

ShaderCode CornerPinDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.frag")),
                    FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.vert")));
}

QPointF CornerPinDistortNode::ValueToPixel(int value, const NodeValueRow& row, const QVector2D &resolution) const
{
  Q_ASSERT(value >= 0 && value <= 3);
  switch (value) {
    case 0: // Top left
      return QPointF(row[kTopLeftInput].data().value<QVector2D>().x(),
                     row[kTopLeftInput].data().value<QVector2D>().y());
      break;
    case 1: // Top right
      return QPointF(resolution.x() + row[kTopRightInput].data().value<QVector2D>().x(),
                     row[kTopRightInput].data().value<QVector2D>().y());
      break;
    case 2: // Bottom right
      return QPointF(resolution.x() + row[kBottomRightInput].data().value<QVector2D>().x(),
                     resolution.y() + row[kBottomRightInput].data().value<QVector2D>().y());
      break;
    case 3: //Bottom left
      return QPointF(row[kBottomLeftInput].data().value<QVector2D>().x(),
                     row[kBottomLeftInput].data().value<QVector2D>().y() + resolution.y());
      break;
    default: // We should never get here
      return QPointF();
  }
}

void CornerPinDistortNode::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  const QVector2D &resolution = globals.resolution();

  const double handle_radius = GetGizmoHandleRadius(p->transform());

  p->setPen(QPen(Qt::white, 0));

  QPointF top_left = ValueToPixel(0, row, resolution);
  QPointF top_right = ValueToPixel(1, row, resolution);
  QPointF bottom_right = ValueToPixel(2, row, resolution);
  QPointF bottom_left = ValueToPixel(3, row, resolution);

  // Add the correct offset to each slider
  SetInputProperty(kTopLeftInput, QStringLiteral("offset"), QVector2D(0.0, 0.0));
  SetInputProperty(kTopRightInput, QStringLiteral("offset"), QVector2D(resolution.x() , 0.0));
  SetInputProperty(kBottomRightInput, QStringLiteral("offset"), resolution);
  SetInputProperty(kBottomLeftInput, QStringLiteral("offset"), QVector2D(0.0, resolution.y()));

  // Draw bounding box
  p->drawLine(QLineF(top_left, top_right));
  p->drawLine(QLineF(top_right, bottom_right));
  p->drawLine(QLineF(bottom_right, bottom_left));
  p->drawLine(QLineF(bottom_left, top_left));

  // Create handles
  gizmo_resize_handle_[0] = CreateGizmoHandleRect(top_left, handle_radius);
  gizmo_resize_handle_[1] = CreateGizmoHandleRect(top_right, handle_radius);
  gizmo_resize_handle_[2] = CreateGizmoHandleRect(bottom_right, handle_radius);
  gizmo_resize_handle_[3] = CreateGizmoHandleRect(bottom_left, handle_radius);

  // Draw handles
  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_resize_handle_, kGizmoCornerCount);
}

bool CornerPinDistortNode::GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p)
{
  bool gizmo_active[kGizmoCornerCount] = {false};

  for (int i = 0; i < kGizmoCornerCount; i++) {
    gizmo_active[i] = gizmo_resize_handle_[i].contains(p);

    if (gizmo_active[i]) {
      gizmo_drag_start_ = p;
      gizmo_res_ = globals.resolution();
      gizmo_drag_ = i;

      switch (i) {
        case 0:
          gizmo_start_ = row[kTopLeftInput].data();
          break;
        case 1:
          gizmo_start_ = row[kTopRightInput].data();
          break;
        case 2:
          gizmo_start_ = row[kBottomRightInput].data();
          break;
        case 3:
          gizmo_start_ = row[kBottomLeftInput].data();
          break;
      }
      return true;
    }
  }

  return false;
}

void CornerPinDistortNode::GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers)
{
  if (gizmo_dragger_.isEmpty()) {
    gizmo_dragger_.resize(2);
    if (gizmo_drag_ == 0) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 1), time);
    }
    if (gizmo_drag_ == 1) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 1), time);
    }
    if (gizmo_drag_ == 2) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 1), time);
    }
    if (gizmo_drag_ == 3) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 1), time);
    }
  }

  double x_diff = (p.x() - gizmo_drag_start_.x());
  double y_diff = (p.y() - gizmo_drag_start_.y());

  QVector2D move = QVector2D(gizmo_start_.value<QVector2D>().x() + x_diff, gizmo_start_.value<QVector2D>().y() + y_diff);

  gizmo_dragger_[0].Drag(move.x());
  gizmo_dragger_[1].Drag(move.y());

}

void CornerPinDistortNode::GizmoRelease(MultiUndoCommand *command) {
  for (NodeInputDragger &i : gizmo_dragger_) {
    i.End(command);
  }
  gizmo_dragger_.clear();

  gizmo_start_.clear();
}

}

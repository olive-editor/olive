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

#define super Node

CornerPinDistortNode::CornerPinDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  AddInput(kPerspectiveInput, NodeValue::kBoolean, true);
  AddInput(kTopLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kTopRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));

  // Initiate gizmos
  gizmo_whole_rect_ = AddDraggableGizmo<PolygonGizmo>();
  gizmo_resize_handle_[0] = AddDraggableGizmo<PointGizmo>({NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 0), NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 1)});
  gizmo_resize_handle_[1] = AddDraggableGizmo<PointGizmo>({NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 0), NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 1)});
  gizmo_resize_handle_[2] = AddDraggableGizmo<PointGizmo>({NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 0), NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 1)});
  gizmo_resize_handle_[3] = AddDraggableGizmo<PointGizmo>({NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 0), NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 1)});

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void CornerPinDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kPerspectiveInput, tr("Perspective"));
  SetInputName(kTopLeftInput, tr("Top Left"));
  SetInputName(kTopRightInput, tr("Top Right"));
  SetInputName(kBottomRightInput, tr("Bottom Right"));
  SetInputName(kBottomLeftInput, tr("Bottom Left"));
}

void CornerPinDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // If no texture do nothing
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    // In the special case that all sliders are in their default position just
    // push the texture.
    if (!(value[kTopLeftInput].toVec2().isNull()
        && value[kTopRightInput].toVec2().isNull() &&
        value[kBottomRightInput].toVec2().isNull() &&
        value[kBottomLeftInput].toVec2().isNull())) {
      ShaderJob job(value);
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));

      // Convert slider values to their pixel values and then convert to clip space (-1.0 ... 1.0) for overriding the
      // vertex coordinates.
      const QVector2D &resolution = tex->virtual_resolution();
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

      table->Push(NodeValue::kTexture, tex->toJob(job), this);
    } else {
      table->Push(value[kTextureInput]);
    }
  }
}

ShaderCode CornerPinDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)

  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.frag")),
                    FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.vert")));
}

QPointF CornerPinDistortNode::ValueToPixel(int value, const NodeValueRow& row, const QVector2D &resolution) const
{
  Q_ASSERT(value >= 0 && value <= 3);

  QVector2D v;

  switch (value) {
  case 0: // Top left
    v = row[kTopLeftInput].toVec2();
    return QPointF(v.x(), v.y());
  case 1: // Top right
    v = row[kTopRightInput].toVec2();
    return QPointF(resolution.x() + v.x(), v.y());
  case 2: // Bottom right
    v = row[kBottomRightInput].toVec2();
    return QPointF(resolution.x() + v.x(), resolution.y() + v.y());
  case 3: //Bottom left
    v = row[kBottomLeftInput].toVec2();
    return QPointF(v.x(), v.y() + resolution.y());
  default: // We should never get here
    return QPointF();
  }
}

void CornerPinDistortNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  if (gizmo != gizmo_whole_rect_) {
    gizmo->GetDraggers()[0].Drag(gizmo->GetDraggers()[0].GetStartValue().toDouble() + x);
    gizmo->GetDraggers()[1].Drag(gizmo->GetDraggers()[1].GetStartValue().toDouble() + y);
  }
}

void CornerPinDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  if (TexturePtr tex = row[kTextureInput].toTexture()) {
    const QVector2D &resolution = tex->virtual_resolution();

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
    gizmo_whole_rect_->SetPolygon(QPolygonF({top_left, top_right, bottom_right, bottom_left, top_left}));

    // Create handles
    gizmo_resize_handle_[0]->SetPoint(top_left);
    gizmo_resize_handle_[1]->SetPoint(top_right);
    gizmo_resize_handle_[2]->SetPoint(bottom_right);
    gizmo_resize_handle_[3]->SetPoint(bottom_left);
  }
}

}

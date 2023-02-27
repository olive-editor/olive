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

#include "swirldistortnode.h"

namespace olive {

const QString SwirlDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString SwirlDistortNode::kRadiusInput = QStringLiteral("radius_in");
const QString SwirlDistortNode::kAngleInput = QStringLiteral("angle_in");
const QString SwirlDistortNode::kPositionInput = QStringLiteral("pos_in");

#define super Node

SwirlDistortNode::SwirlDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kRadiusInput, NodeValue::kFloat, 200);
  SetInputProperty(kRadiusInput, QStringLiteral("min"), 0);

  AddInput(kAngleInput, NodeValue::kFloat, 10);
  SetInputProperty(kAngleInput, QStringLiteral("base"), 0.1);

  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);

  gizmo_ = AddDraggableGizmo<PointGizmo>({
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1),
  });
  gizmo_->SetShape(PointGizmo::kAnchorPoint);
}

QString SwirlDistortNode::Name() const
{
  return tr("Swirl");
}

QString SwirlDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.swirl");
}

QVector<Node::CategoryID> SwirlDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString SwirlDistortNode::Description() const
{
  return tr("Distorts an image along a sine wave.");
}

void SwirlDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kRadiusInput, tr("Radius"));
  SetInputName(kAngleInput, tr("Angle"));
  SetInputName(kPositionInput, tr("Position"));
}

ShaderCode SwirlDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/swirl.frag"));
}

void SwirlDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // If there's no texture, no need to run an operation
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    // Only run shader if at least one of flip or flop are selected
    if (!qIsNull(value[kAngleInput].toDouble()) && !qIsNull(value[kRadiusInput].toDouble())) {
      ShaderJob job(value);
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
      table->Push(NodeValue::kTexture, tex->toJob(job), this);
    } else {
      // If we're not flipping or flopping just push the texture
      table->Push(value[kTextureInput]);
    }
  }
}

void SwirlDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  QPointF half_res(globals.square_resolution().x()/2, globals.square_resolution().y()/2);

  gizmo_->SetPoint(half_res + row[kPositionInput].toVec2().toPointF());
}

void SwirlDistortNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  NodeInputDragger &x_drag = gizmo_->GetDraggers()[0];
  NodeInputDragger &y_drag = gizmo_->GetDraggers()[1];

  x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
  y_drag.Drag(y_drag.GetStartValue().toDouble() + y);
}

}

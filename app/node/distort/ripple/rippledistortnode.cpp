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

#include "rippledistortnode.h"

namespace olive {

const QString RippleDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString RippleDistortNode::kEvolutionInput = QStringLiteral("evolution_in");
const QString RippleDistortNode::kIntensityInput = QStringLiteral("intensity_in");
const QString RippleDistortNode::kFrequencyInput = QStringLiteral("frequency_in");
const QString RippleDistortNode::kPositionInput = QStringLiteral("position_in");
const QString RippleDistortNode::kStretchInput = QStringLiteral("stretch_in");

#define super Node

RippleDistortNode::RippleDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kEvolutionInput, NodeValue::kFloat, 0);
  AddInput(kIntensityInput, NodeValue::kFloat, 100);

  AddInput(kFrequencyInput, NodeValue::kFloat, 1);
  SetInputProperty(kFrequencyInput, QStringLiteral("base"), 0.01);

  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));
  AddInput(kStretchInput, NodeValue::kBoolean, false);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);

  gizmo_ = AddDraggableGizmo<PointGizmo>({
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1),
  });
  gizmo_->SetShape(PointGizmo::kAnchorPoint);
}

QString RippleDistortNode::Name() const
{
  return tr("Ripple");
}

QString RippleDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.ripple");
}

QVector<Node::CategoryID> RippleDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString RippleDistortNode::Description() const
{
  return tr("Distorts an image with a ripple effect.");
}

void RippleDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kFrequencyInput, tr("Frequency"));
  SetInputName(kIntensityInput, tr("Intensity"));
  SetInputName(kEvolutionInput, tr("Evolution"));
  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kStretchInput, tr("Stretch"));
}

ShaderCode RippleDistortNode::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/ripple.frag"));
}

NodeValue RippleDistortNode::Value(const ValueParams &p) const
{
  // If there's no texture, no need to run an operation
  NodeValue texture = GetInputValue(p, kTextureInput);

  if (TexturePtr tex = texture.toTexture()) {
    // Only run shader if at least one of flip or flop are selected
    NodeValue intensity = GetInputValue(p, kIntensityInput);

    if (!qIsNull(intensity.toDouble())) {
      ShaderJob job;

      job.set_function(GetShaderCode);
      job.Insert(kTextureInput, texture);
      job.Insert(kEvolutionInput, GetInputValue(p, kEvolutionInput));
      job.Insert(kIntensityInput, intensity);
      job.Insert(kFrequencyInput, GetInputValue(p, kFrequencyInput));
      job.Insert(kPositionInput, GetInputValue(p, kPositionInput));
      job.Insert(kStretchInput, GetInputValue(p, kStretchInput));
      job.Insert(QStringLiteral("resolution_in"), tex->virtual_resolution());

      return tex->toJob(job);
    } else {
      // If we're not flipping or flopping just push the texture
      return texture;
    }
  }

  return NodeValue();
}

void RippleDistortNode::UpdateGizmoPositions(const ValueParams &p)
{
  if (TexturePtr tex = GetInputValue(p, kTextureInput).toTexture()) {
    QPointF half_res(tex->virtual_resolution().x()/2, tex->virtual_resolution().y()/2);
    gizmo_->SetPoint(half_res + GetInputValue(p, kPositionInput).toVec2().toPointF());
  }
}

void RippleDistortNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  NodeInputDragger &x_drag = gizmo_->GetDraggers()[0];
  NodeInputDragger &y_drag = gizmo_->GetDraggers()[1];

  x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
  y_drag.Drag(y_drag.GetStartValue().toDouble() + y);
}

}

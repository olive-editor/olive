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

#include "blur.h"

namespace olive {

const QString BlurFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString BlurFilterNode::kMethodInput = QStringLiteral("method_in");
const QString BlurFilterNode::kRadiusInput = QStringLiteral("radius_in");
const QString BlurFilterNode::kHorizInput = QStringLiteral("horiz_in");
const QString BlurFilterNode::kVertInput = QStringLiteral("vert_in");
const QString BlurFilterNode::kRepeatEdgePixelsInput = QStringLiteral("repeat_edge_pixels_in");

const QString BlurFilterNode::kDirectionalDegreesInput = QStringLiteral("directional_degrees_in");

const QString BlurFilterNode::kRadialCenterInput = QStringLiteral("radial_center_in");

#define super Node

BlurFilterNode::BlurFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  Method default_method = kGaussian;

  AddInput(kMethodInput, NodeValue::kCombo, default_method, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

  AddInput(kRadiusInput, NodeValue::kFloat, 10.0);
  SetInputProperty(kRadiusInput, QStringLiteral("min"), 0.0);

  {
    // Box and gaussian only
    AddInput(kHorizInput, NodeValue::kBoolean, true);
    AddInput(kVertInput, NodeValue::kBoolean, true);
  }

  {
    // Directional only
    AddInput(kDirectionalDegreesInput, NodeValue::kFloat, 0.0);
  }

  {
    // Radial only
    AddInput(kRadialCenterInput, NodeValue::kVec2, QVector2D(0, 0));
  }

  UpdateInputs(default_method);

  AddInput(kRepeatEdgePixelsInput, NodeValue::kBoolean, true);

  SetFlags(kVideoEffect);
  SetEffectInput(kTextureInput);

  radial_center_gizmo_ = AddDraggableGizmo<PointGizmo>();
  radial_center_gizmo_->SetShape(PointGizmo::kAnchorPoint);
  radial_center_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kRadialCenterInput), 0));
  radial_center_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kRadialCenterInput), 1));
}

QString BlurFilterNode::Name() const
{
  return tr("Blur");
}

QString BlurFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.blur");
}

QVector<Node::CategoryID> BlurFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString BlurFilterNode::Description() const
{
  return tr("Blurs an image.");
}

void BlurFilterNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kMethodInput, tr("Method"));
  SetComboBoxStrings(kMethodInput, { tr("Box"), tr("Gaussian"), tr("Directional"), tr("Radial") });
  SetInputName(kRadiusInput, tr("Radius"));
  SetInputName(kHorizInput, tr("Horizontal"));
  SetInputName(kVertInput, tr("Vertical"));
  SetInputName(kRepeatEdgePixelsInput, tr("Repeat Edge Pixels"));

  SetInputName(kDirectionalDegreesInput, tr("Direction"));
  SetInputName(kRadialCenterInput, tr("Center"));
}

ShaderCode BlurFilterNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/blur.frag"));
}

void BlurFilterNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;

  job.Insert(value);
  job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  Method method = static_cast<Method>(job.Get(kMethodInput).toInt());

  // If there's no texture, no need to run an operation
  if (job.Get(kTextureInput).toTexture()) {

    bool can_push_job = true;

    // Check if radius is > 0
    if (job.Get(kRadiusInput).toDouble() > 0.0) {
      // Method-specific considerations
      switch (method) {
      case kBox:
      case kGaussian:
      {
        bool horiz = job.Get(kHorizInput).toBool();
        bool vert = job.Get(kVertInput).toBool();

        if (!horiz && !vert) {
          // Disable job if horiz and vert are unchecked
          can_push_job = false;
        } else if (horiz && vert) {
          // Set iteration count to 2 if we're blurring both horizontally and vertically
          job.SetIterations(2, kTextureInput);
        }
        break;
      }
      case kDirectional:
      case kRadial:
        break;
      }
    } else {
      can_push_job = false;
    }

    if (can_push_job) {
      // If we're not repeating pixels, expect an alpha channel to appear
      if (!job.Get(kRepeatEdgePixelsInput).toBool()) {
        job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
      }

      table->Push(NodeValue::kTexture, QVariant::fromValue(job), this);
    } else {
      // If we're not performing the blur job, just push the texture
      table->Push(job.Get(kTextureInput));
    }

  }
}

void BlurFilterNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  if (row[kMethodInput].toInt() == kRadial) {
    const QVector2D &sequence_res = globals.resolution();
    QVector2D sequence_half_res = sequence_res * 0.5;

    radial_center_gizmo_->SetVisible(true);
    radial_center_gizmo_->SetPoint(sequence_half_res.toPointF() + row[kRadialCenterInput].toVec2().toPointF());

    SetInputProperty(kRadialCenterInput, QStringLiteral("offset"), sequence_half_res);
  } else{
    radial_center_gizmo_->SetVisible(false);
  }
}

void BlurFilterNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  if (gizmo == radial_center_gizmo_) {

    NodeInputDragger &x_drag = gizmo->GetDraggers()[0];
    NodeInputDragger &y_drag = gizmo->GetDraggers()[1];

    x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
    y_drag.Drag(y_drag.GetStartValue().toDouble() + y);

  }
}

void BlurFilterNode::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kMethodInput) {
    UpdateInputs(GetMethod());
  }

  super::InputValueChangedEvent(input, element);
}

void BlurFilterNode::UpdateInputs(Method method)
{
  SetInputFlags(kHorizInput, (method == kBox || method == kGaussian) ? InputFlags() : InputFlags(kInputFlagHidden));
  SetInputFlags(kVertInput, (method == kBox || method == kGaussian) ? InputFlags() : InputFlags(kInputFlagHidden));
  SetInputFlags(kDirectionalDegreesInput, (method == kDirectional) ? InputFlags() : InputFlags(kInputFlagHidden));
  SetInputFlags(kRadialCenterInput, (method == kRadial) ? InputFlags() : InputFlags(kInputFlagHidden));
}

}

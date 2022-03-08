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

#include "displaytransform.h"

#include "node/project/project.h"
#include "render/colorprocessor.h"

namespace olive {

const QString DisplayTransformNode::kTextureInput = QStringLiteral("tex_in");
const QString DisplayTransformNode::kDisplayInput = QStringLiteral("display_in");
const QString DisplayTransformNode::kViewInput = QStringLiteral("view_in");
const QString DisplayTransformNode::kDirectionInput = QStringLiteral("dir_in");

DisplayTransformNode::DisplayTransformNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kDisplayInput, NodeValue::kCombo, 0);

  AddInput(kViewInput, NodeValue::kCombo, 0);

  AddInput(kDirectionInput, NodeValue::kCombo, 0);

  Retranslate();
}

void DisplayTransformNode::GenerateProcessor()
{
  if (!project()) {
    return;
  }
  if (view_.isEmpty()) {
    Retranslate();
  }

  ColorTransform transform(display_, view_, "");

  reference_to_display_ = ColorProcessor::Create(project()->color_manager(),
                                                   project()->color_manager()->GetReferenceColorSpace(), transform);

  // Create shader description
  shader_desc_ = OCIO::GpuShaderDesc::CreateShaderDesc();
  shader_desc_->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
  shader_desc_->setFunctionName("DisplayTransform");
  shader_desc_->setResourcePrefix("ocio_");

  // Generate shader
  reference_to_display_->GetProcessor()->getDefaultGPUProcessor()->extractGpuShaderInfo(shader_desc_);
}

Node *DisplayTransformNode::copy() const
{
  return new DisplayTransformNode();
}

QString DisplayTransformNode::Name() const
{
  return tr("Display Transform");
}

QString DisplayTransformNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.disaplaytransform");
}

QVector<Node::CategoryID> DisplayTransformNode::Category() const
{
  return {kCategoryColor};
}

QString DisplayTransformNode::Description() const
{
  return tr("Converts an image to/from display space");
}

void DisplayTransformNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kDisplayInput, tr("Display"));
  SetInputName(kViewInput, tr("View"));

  if (project()) {
    SetComboBoxStrings(kDisplayInput, project()->color_manager()->ListAvailableDisplays());
    display_ = project()->color_manager()->ListAvailableDisplays().at(
                          GetSplitStandardValue(kDisplayInput).at(0).toString().toInt());

    SetComboBoxStrings(kViewInput, project()->color_manager()->ListAvailableViews(display_));
    view_ = project()->color_manager()->ListAvailableViews(display_).at(
                       GetSplitStandardValue(kViewInput).at(0).toString().toInt());
  }

  SetInputName(kDirectionInput, tr("Direction"));
  SetComboBoxStrings(kDirectionInput, {tr("Forward"), tr("Inverse")});
}

void DisplayTransformNode::InputValueChangedEvent(const QString &input, int element) {
  Q_UNUSED(element);
  if (input == kDirectionInput) {
    GenerateProcessor();
  }
  if (project()) {
    if (input == kDisplayInput) {
      display_ = project()->color_manager()->ListAvailableDisplays().at(
                            GetSplitStandardValue(kDisplayInput).at(0).toString().toInt());
      // If we change the display the view menu must be updated
      Retranslate();
      GenerateProcessor();
    }
    if (input == kViewInput) {
      view_ = project()->color_manager()->ListAvailableViews(display_).at(
                         GetSplitStandardValue(kViewInput).at(0).toString().toInt());
      GenerateProcessor();
    }
  }

}

ShaderCode DisplayTransformNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  // Generate shader code using OCIO stub and our auto-generated name
  QString shader_frag = FileFunctions::ReadFileAsString(QStringLiteral(":shaders/displaytransform.frag"))
                            .arg(shader_desc_->getShaderText());
  return ShaderCode(shader_frag);
}

void DisplayTransformNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.SetUseOCIO(true);
  job.SetShaderDesc(shader_desc_);
  job.SetColorProcessor(reference_to_display_);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  }
}

}

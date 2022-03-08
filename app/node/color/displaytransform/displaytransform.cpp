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
const QString DisplayTransformNode::kDirectionInput = QStringLiteral("dir_in");

DisplayTransformNode::DisplayTransformNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kDirectionInput, NodeValue::kCombo, 0);

  GenerateProcessor(true);
}

void DisplayTransformNode::GenerateProcessor(bool direction)
{
  if (!project()) {
    return;
  }
  qDebug() << project()->color_manager()->GetReferenceColorSpace() << project()->color_manager()->GetDefaultDisplay();

  ColorTransform transform("sRGB OETF");
  reference_to_display_ = ColorProcessor::Create(project()->color_manager(), project()->color_manager()->GetReferenceColorSpace(),
                                                 transform);

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
  SetInputName(kDirectionInput, tr("Direction"));
  SetComboBoxStrings(kDirectionInput, {tr("Forward"), tr("Inverse")});
  GenerateProcessor(true);
}

void DisplayTransformNode::InputValueChangedEvent(const QString& input, int element)
{
    Q_UNUSED(element);
  if (input == kDirectionInput) {
      GenerateProcessor(true);
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
  
  //renderer()->ShaderJobInsertTextures(reference_to_display_, &job, shader_desc_);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  }
}

}

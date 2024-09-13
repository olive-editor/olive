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

#include "dilateerode.h"

namespace olive {

const QString DilateErodeFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString DilateErodeFilterNode::kMethodInput = QStringLiteral("method_in");
const QString DilateErodeFilterNode::kPixelsInput = QStringLiteral("pixels_in");

#define super Node

DilateErodeFilterNode::DilateErodeFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kMethodInput, NodeValue::kCombo, 0);

  AddInput(kPixelsInput, NodeValue::kInt, 0);

  SetFlags(kVideoEffect);
  SetEffectInput(kTextureInput);
}

Node* DilateErodeFilterNode::copy() const
{
  return new DilateErodeFilterNode();
}

QString DilateErodeFilterNode::Name() const
{
  return tr("Dilate/Erode");
}

QString DilateErodeFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.dilateerode");
}

QVector<Node::CategoryID> DilateErodeFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString DilateErodeFilterNode::Description() const
{
  return tr("Grows or shrinks bright areas by the set number of pixels");
}

void DilateErodeFilterNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kMethodInput, tr("Method"));
  SetComboBoxStrings(kMethodInput, {tr("Box"), tr("Distance"), tr("Gaussian")});
  SetInputName(kPixelsInput, tr("Pixels"));
}

ShaderCode DilateErodeFilterNode::GetShaderCode(const ShaderRequest& request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/dilateerode.frag"));
}

void DilateErodeFilterNode::Value(const NodeValueRow& value, const NodeGlobals& globals, NodeValueTable* table) const
{
  // If there's no texture and no dilation/erosion, no need to run an operation
  if (value[kTextureInput].toTexture() && value[kPixelsInput].data().toInt() != 0) {
    TexturePtr tex = value[kTextureInput].toTexture();
    //job.SetIterations(2, kTextureInput);
    ShaderJob job(value);
    job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
    job.SetIterations(2, kTextureInput);
    table->Push(NodeValue::kTexture, tex->toJob(job), this);
  } else {
    // If we're not doing anything just push the texture
    table->Push(value[kTextureInput]);
  }
}

}

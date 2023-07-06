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

#include "mask.h"

#include "node/filter/blur/blur.h"

namespace olive {

#define super PolygonGenerator

const QString MaskDistortNode::kFeatherInput = QStringLiteral("feather_in");
const QString MaskDistortNode::kInvertInput = QStringLiteral("invert_in");

MaskDistortNode::MaskDistortNode()
{
  // Mask should always be (1.0, 1.0, 1.0) for multiply to work correctly
  SetInputFlag(kColorInput, kInputFlagHidden);

  AddInput(kInvertInput, TYPE_BOOL, false);

  AddInput(kFeatherInput, TYPE_DOUBLE, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);
}

ShaderCode GetInvertShader(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/invertrgba.frag")));
}

ShaderCode GetFeatherShader(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/blur.frag")));
}

ShaderCode GetMergeShader(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/multiply.frag")));
}

void MaskDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseInput, tr("Texture"));
  SetInputName(kInvertInput, tr("Invert"));
  SetInputName(kFeatherInput, tr("Feather"));
}

value_t MaskDistortNode::Value(const ValueParams &p) const
{
  TexturePtr texture = GetInputValue(p, kBaseInput).toTexture();

  VideoParams job_params = texture ? texture->params() : p.vparams();
  value_t job = Texture::Job(job_params, GetGenerateJob(p, job_params));

  if (GetInputValue(p, kInvertInput).toBool()) {
    ShaderJob invert;
    invert.set_function(GetInvertShader);
    invert.Insert(QStringLiteral("tex_in"), job);
    job = Texture::Job(job_params, invert);
  }

  if (texture) {
    // Push as merge node
    ShaderJob merge;

    merge.set_function(GetMergeShader);
    merge.Insert(QStringLiteral("tex_a"), GetInputValue(p, kBaseInput));

    if (GetInputValue(p, kFeatherInput).toDouble() > 0.0) {
      // Nest a blur shader in there too
      ShaderJob feather;

      feather.set_function(GetFeatherShader);
      feather.Insert(BlurFilterNode::kTextureInput, job);
      feather.Insert(BlurFilterNode::kMethodInput, int(BlurFilterNode::kGaussian));
      feather.Insert(BlurFilterNode::kHorizInput, true);
      feather.Insert(BlurFilterNode::kVertInput, true);
      feather.Insert(BlurFilterNode::kRepeatEdgePixelsInput, true);
      feather.Insert(BlurFilterNode::kRadiusInput, GetInputValue(p, kFeatherInput).toDouble());
      feather.SetIterations(2, BlurFilterNode::kTextureInput);
      feather.Insert(QStringLiteral("resolution_in"), texture ? texture->virtual_resolution() : p.square_resolution());

      merge.Insert(QStringLiteral("tex_b"), Texture::Job(job_params, feather));
    } else {
      merge.Insert(QStringLiteral("tex_b"), job);
    }

    return Texture::Job(job_params, merge);
  } else {
    return job;
  }
}

}

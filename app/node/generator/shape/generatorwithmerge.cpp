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

#include "generatorwithmerge.h"

#include "node/math/merge/merge.h"

namespace olive {

#define super Node

const QString GeneratorWithMerge::kBaseInput = QStringLiteral("base_in");

GeneratorWithMerge::GeneratorWithMerge()
{
  AddInput(kBaseInput, TYPE_TEXTURE, kInputFlagNotKeyframable);
  SetEffectInput(kBaseInput);
  SetFlag(kVideoEffect);
}

void GeneratorWithMerge::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseInput, tr("Base"));
}

ShaderCode GeneratorWithMerge::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/alphaover.frag"));
}

value_t GeneratorWithMerge::GetMergableJob(const ValueParams &p, TexturePtr job) const
{
  value_t tex_meta = GetInputValue(p, kBaseInput);

  if (TexturePtr base = tex_meta.toTexture()) {
    // Push as merge node
    ShaderJob merge;

    merge.SetShaderID(QStringLiteral("mrg"));
    merge.set_function(GetShaderCode);
    merge.Insert(MergeNode::kBaseIn, tex_meta);
    merge.Insert(MergeNode::kBlendIn, job);

    return base->toJob(merge);
  } else {
    // Just push generate job
    return job;
  }
}

}

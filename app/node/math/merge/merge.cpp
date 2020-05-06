/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "merge.h"

OLIVE_NAMESPACE_ENTER

MergeNode::MergeNode()
{
  base_in_ = new NodeInput("base_in", NodeParam::kTexture);
  AddInput(base_in_);

  blend_in_ = new NodeInput("blend_in", NodeParam::kTexture);
  AddInput(blend_in_);
}

Node *MergeNode::copy() const
{
  return new MergeNode();
}

QString MergeNode::Name() const
{
  return tr("Merge");
}

QString MergeNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.merge");
}

QList<Node::CategoryID> MergeNode::Category() const
{
  return {kCategoryMath};
}

QString MergeNode::Description() const
{
  return tr("Merge two textures together.");
}

void MergeNode::Retranslate()
{
  base_in_->set_name(tr("Base"));
  blend_in_->set_name(tr("Blend"));
}

Node::Capabilities MergeNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString MergeNode::ShaderFragmentCode(const NodeValueDatabase &) const
{
  return ReadFileAsString(":/shaders/alphaover.frag");
}

NodeInput *MergeNode::base_in() const
{
  return base_in_;
}

NodeInput *MergeNode::blend_in() const
{
  return blend_in_;
}

OLIVE_NAMESPACE_EXIT

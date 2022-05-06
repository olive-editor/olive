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

#include "ociobase.h"

#include "node/color/colormanager/colormanager.h"
#include "node/project/project.h"

namespace olive {

const QString OCIOBaseNode::kTextureInput = QStringLiteral("tex_in");

OCIOBaseNode::OCIOBaseNode() :
  manager_(nullptr),
  processor_(nullptr)
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  SetEffectInput(kTextureInput);

  connect(this, &Node::AddedToGraph, this, &OCIOBaseNode::ParentChanged);
}

void OCIOBaseNode::ParentChanged(NodeGraph *graph)
{
  if (manager_) {
    disconnect(manager_, &ColorManager::ConfigChanged, this, &OCIOBaseNode::ConfigChanged);
    manager_ = nullptr;
  }

  if (Project *p = dynamic_cast<Project*>(graph)) {
    manager_ = p->color_manager();
    connect(manager_, &ColorManager::ConfigChanged, this, &OCIOBaseNode::ConfigChanged);
    ConfigChanged();
  }
}

void OCIOBaseNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (!value[kTextureInput].data().isNull() && processor_) {
    ColorTransformJob job;

    job.SetColorProcessor(processor_);
    job.SetInputTexture(value[kTextureInput].data().value<TexturePtr>());

    table->Push(NodeValue::kTexture, QVariant::fromValue(job), this);
  }
}

}

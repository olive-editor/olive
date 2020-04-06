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

#include "externaltransition.h"

OLIVE_NAMESPACE_ENTER

ExternalTransition::ExternalTransition(const QString &xml_meta_filename) :
  meta_(xml_meta_filename)
{
  foreach (NodeInput* input, meta_.inputs()) {
    AddInput(input);
  }
}

Node *ExternalTransition::copy() const
{
  return new ExternalTransition(meta_.filename());
}

QString ExternalTransition::Name() const
{
  return meta_.Name();
}

QString ExternalTransition::id() const
{
  return meta_.id();
}

QString ExternalTransition::Category() const
{
  return meta_.Category();
}

QString ExternalTransition::Description() const
{
  return meta_.Description();
}

void ExternalTransition::Retranslate()
{
  meta_.Retranslate();
}

Node::Capabilities ExternalTransition::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString ExternalTransition::ShaderVertexCode(const NodeValueDatabase &) const
{
  return meta_.vert_code();
}

QString ExternalTransition::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return meta_.frag_code();
}

int ExternalTransition::ShaderIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalTransition::ShaderIterativeInput() const
{
  return meta_.iteration_input();
}

OLIVE_NAMESPACE_EXIT

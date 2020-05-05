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

#include "external.h"

#include <QFile>

OLIVE_NAMESPACE_ENTER

ExternalNode::ExternalNode(const QString &xml_meta_filename) :
  meta_(xml_meta_filename)
{
  foreach (NodeInput* input, meta_.inputs()) {
    AddInput(input);
  }
}

Node *ExternalNode::copy() const
{
  return new ExternalNode(meta_.filename());
}

QString ExternalNode::Name() const
{
  return meta_.Name();
}

QString ExternalNode::ShortName() const
{
  return meta_.ShortName();
}

QString ExternalNode::id() const
{
  return meta_.id();
}

QList<Node::CategoryID> ExternalNode::Category() const
{
  return meta_.Category();
}

QString ExternalNode::Description() const
{
  return meta_.Description();
}

void ExternalNode::Retranslate()
{
  meta_.Retranslate();
}

Node::Capabilities ExternalNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString ExternalNode::ShaderVertexCode(const NodeValueDatabase&) const
{
  return meta_.vert_code();
}

QString ExternalNode::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return meta_.frag_code();
}

int ExternalNode::ShaderIterations() const
{
  return meta_.iterations();
}

NodeInput *ExternalNode::ShaderIterativeInput() const
{
  return meta_.iteration_input();
}

OLIVE_NAMESPACE_EXIT

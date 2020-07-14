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

#include "crossdissolvetransition.h"

OLIVE_NAMESPACE_ENTER

CrossDissolveTransition::CrossDissolveTransition()
{

}

Node *CrossDissolveTransition::copy() const
{
  return new CrossDissolveTransition();
}

QString CrossDissolveTransition::Name() const
{
  return tr("Cross Dissolve");
}

QString CrossDissolveTransition::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.crossdissolve");
}

QList<Node::CategoryID> CrossDissolveTransition::Category() const
{
  return {kCategoryTransition};
}

QString CrossDissolveTransition::Description() const
{
  return tr("Smoothly transition between two clips.");
}

ShaderCode CrossDissolveTransition::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(Node::ReadFileAsString(":/shaders/crossdissolve.frag"), QString());
}

OLIVE_NAMESPACE_EXIT

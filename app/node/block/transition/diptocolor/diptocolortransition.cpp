/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "diptocolortransition.h"

namespace olive {

DipToColorTransition::DipToColorTransition()
{
  color_input_ = new NodeInput(this, QStringLiteral("color_in"), NodeValue::kColor, QVariant::fromValue(Color(0, 0, 0)));
}

Node *DipToColorTransition::copy() const
{
  return new DipToColorTransition();
}

QString DipToColorTransition::Name() const
{
  return tr("Dip To Color");
}

QString DipToColorTransition::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.diptocolor");
}

QVector<Node::CategoryID> DipToColorTransition::Category() const
{
  return {kCategoryTransition};
}

QString DipToColorTransition::Description() const
{
  return tr("Transition between clips by dipping to a color.");
}

ShaderCode DipToColorTransition::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/diptoblack.frag"), QString());
}

void DipToColorTransition::ShaderJobEvent(NodeValueDatabase &value, ShaderJob &job) const
{
  job.InsertValue(color_input_, value);
}

}

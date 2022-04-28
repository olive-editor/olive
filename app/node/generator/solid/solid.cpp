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

#include "solid.h"

#include "render/color.h"

namespace olive {

const QString SolidGenerator::kColorInput = QStringLiteral("color_in");

#define super Node

SolidGenerator::SolidGenerator()
{
  // Default to a color that isn't black
  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0f, 0.0f, 0.0f, 1.0f)));
}

Node *SolidGenerator::copy() const
{
  return new SolidGenerator();
}

QString SolidGenerator::Name() const
{
  return tr("Solid");
}

QString SolidGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.solidgenerator");
}

QVector<Node::CategoryID> SolidGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString SolidGenerator::Description() const
{
  return tr("Generate a solid color.");
}

void SolidGenerator::Retranslate()
{
  super::Retranslate();

  SetInputName(kColorInput, tr("Color"));
}

void SolidGenerator::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
}

ShaderCode SolidGenerator::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/solid.frag"));
}

}

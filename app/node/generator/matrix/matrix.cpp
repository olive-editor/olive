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

#include "matrix.h"

#include <QMatrix4x4>
#include <QVector2D>

OLIVE_NAMESPACE_ENTER

MatrixGenerator::MatrixGenerator()
{
  position_input_ = new NodeInput("pos_in", NodeParam::kVec2);
  AddInput(position_input_);

  rotation_input_ = new NodeInput("rot_in", NodeParam::kFloat);
  AddInput(rotation_input_);

  scale_input_ = new NodeInput("scale_in", NodeParam::kVec2, QVector2D(1.0f, 1.0f));
  scale_input_->set_property("min", QVector2D(0, 0));
  scale_input_->set_property("view", "percent");
  scale_input_->set_property("disabley", true);
  AddInput(scale_input_);

  uniform_scale_input_ = new NodeInput("uniform_scale_in", NodeParam::kBoolean, true);
  uniform_scale_input_->set_is_keyframable(false);
  uniform_scale_input_->SetConnectable(false);
  connect(uniform_scale_input_, &NodeInput::ValueChanged, this, &MatrixGenerator::UniformScaleChanged);
  AddInput(uniform_scale_input_);

  anchor_input_ = new NodeInput("anchor_in", NodeParam::kVec2);
  AddInput(anchor_input_);
}

Node *MatrixGenerator::copy() const
{
  return new MatrixGenerator();
}

QString MatrixGenerator::Name() const
{
  return tr("Orthographic Matrix");
}

QString MatrixGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.transform");
}

QString MatrixGenerator::Category() const
{
  return tr("Generator");
}

QString MatrixGenerator::Description() const
{
  return tr("Generate an orthographic matrix using position, rotation, and scale.");
}

void MatrixGenerator::Retranslate()
{
  position_input_->set_name(tr("Position"));
  rotation_input_->set_name(tr("Rotation"));
  scale_input_->set_name(tr("Scale"));
  uniform_scale_input_->set_name(tr("Uniform Scale"));
  anchor_input_->set_name(tr("Anchor Point"));
}

NodeValueTable MatrixGenerator::Value(NodeValueDatabase &value) const
{
  QMatrix4x4 mat;

  // Position translate
  QVector2D pos = value[position_input_].Get(NodeParam::kVec2).value<QVector2D>();
  mat.translate(pos);

  // Rotation
  mat.rotate(value[rotation_input_].Get(NodeParam::kFloat).toFloat(), 0, 0, 1);

  // Scale and Uniform Scale
  QVector2D scale = value[scale_input_].Get(NodeParam::kVec2).value<QVector2D>();
  if (value[uniform_scale_input_].Get(NodeParam::kBoolean).toBool()) {
    scale.setY(scale.x());
  }
  mat.scale(scale);

  // Anchor Point
  mat.translate(-value[anchor_input_].Get(NodeParam::kVec2).value<QVector2D>());

  // Push matrix output
  NodeValueTable output;
  output.Push(NodeParam::kMatrix, mat);
  return output;
}

void MatrixGenerator::UniformScaleChanged()
{
  scale_input_->set_property("disabley", uniform_scale_input_->get_standard_value().toBool());
}

OLIVE_NAMESPACE_EXIT

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

#include "transform.h"

#include <QMatrix4x4>
#include <QVector2D>

TransformDistort::TransformDistort()
{
  position_input_ = new NodeInput("pos_in");
  position_input_->set_data_type(NodeParam::kVec2);
  AddInput(position_input_);

  rotation_input_ = new NodeInput("rot_in");
  rotation_input_->set_data_type(NodeParam::kFloat);
  AddInput(rotation_input_);

  scale_input_ = new NodeInput("scale_in");
  scale_input_->set_data_type(NodeParam::kVec2);
  scale_input_->set_value_at_time(0, QVector2D(100.0f, 100.0f));
  AddInput(scale_input_);

  anchor_input_ = new NodeInput("anchor_in");
  anchor_input_->set_data_type(NodeParam::kVec2);
  AddInput(anchor_input_);
}

Node *TransformDistort::copy() const
{
  return new TransformDistort();
}

QString TransformDistort::Name() const
{
  return tr("Transform");
}

QString TransformDistort::id() const
{
  return "org.olivevideoeditor.Olive.transform";
}

QString TransformDistort::Category() const
{
  return tr("Distort");
}

QString TransformDistort::Description() const
{
  return tr("Apply transformations to position, rotation, and scale.");
}

void TransformDistort::Retranslate()
{
  position_input_->set_name(tr("Position"));
  rotation_input_->set_name(tr("Rotation"));
  scale_input_->set_name(tr("Scale"));
  anchor_input_->set_name(tr("Anchor Point"));
}

NodeValueTable TransformDistort::Value(const NodeValueDatabase &value) const
{
  QMatrix4x4 mat;

  // Position translate
  QVector2D pos = value[position_input_].Get(NodeParam::kVec2).value<QVector2D>();
  mat.translate(pos);

  // Rotation
  mat.rotate(value[rotation_input_].Get(NodeParam::kFloat).toFloat(), 0, 0, 1);

  // Scale
  mat.scale(value[scale_input_].Get(NodeParam::kVec2).value<QVector2D>()*0.01f);

  // Anchor Point
  mat.translate(-value[anchor_input_].Get(NodeParam::kVec2).value<QVector2D>());

  // Push matrix output
  NodeValueTable output = value.Merge();
  output.Push(NodeParam::kMatrix, mat);
  return output;
}

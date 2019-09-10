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
  position_input_->add_data_input(NodeParam::kVec2);
  AddParameter(position_input_);

  rotation_input_ = new NodeInput("rot_in");
  rotation_input_->add_data_input(NodeParam::kFloat);
  AddParameter(rotation_input_);

  scale_input_ = new NodeInput("scale_in");
  scale_input_->add_data_input(NodeParam::kVec2);
  scale_input_->set_value(QVector2D(100.0f, 100.0f));
  AddParameter(scale_input_);

  anchor_input_ = new NodeInput("anchor_in");
  anchor_input_->add_data_input(NodeParam::kVec2);
  AddParameter(anchor_input_);

  matrix_output_ = new NodeOutput("matrix_out");
  matrix_output_->set_data_type(NodeParam::kMatrix);
  AddParameter(matrix_output_);
}

QString TransformDistort::Name()
{
  return tr("Transform");
}

QString TransformDistort::id()
{
  return "org.olivevideoeditor.Olive.transform";
}

QString TransformDistort::Category()
{
  return tr("Distort");
}

QString TransformDistort::Description()
{
  return tr("Apply transformations to position, rotation, and scale.");
}

NodeOutput *TransformDistort::matrix_output()
{
  return matrix_output_;
}

void TransformDistort::Retranslate()
{
  position_input_->set_name(tr("Position"));
  rotation_input_->set_name(tr("Rotation"));
  scale_input_->set_name(tr("Scale"));
  anchor_input_->set_name(tr("Anchor Point"));
}

QVariant TransformDistort::Value(NodeOutput *output, const rational &time)
{
  if (output == matrix_output_) {
    QMatrix4x4 mat;

    // Position translate
    QVector2D pos = position_input_->get_value(time).value<QVector2D>();
    mat.translate(pos);

    // Rotation
    mat.rotate(rotation_input_->get_value(time).toFloat(), 0, 0, 1);

    // Scale
    mat.scale(scale_input_->get_value(time).value<QVector2D>()*0.01f);

    // Anchor Point
    mat.translate(anchor_input_->get_value(time).value<QVector2D>());

    return mat;
  }

  return 0;
}

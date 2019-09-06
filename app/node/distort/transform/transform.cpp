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

QVariant TransformDistort::Value(NodeOutput *output, const rational &time)
{
  if (output == matrix_output_) {
    QMatrix4x4 mat;

    mat.translate(position_input_->get_value(time).value<QVector2D>());
    //mat.rotate(0)
    //mat.scale()

    return mat;
  }

  return 0;
}

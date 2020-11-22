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

#include "matrix.h"

#include <QMatrix4x4>
#include <QVector2D>

namespace olive {

MatrixGenerator::MatrixGenerator()
{
  position_input_ = new NodeInput("pos_in", NodeParam::kVec2, QVector2D());
  AddInput(position_input_);

  rotation_input_ = new NodeInput("rot_in", NodeParam::kFloat, 0.0f);
  AddInput(rotation_input_);

  scale_input_ = new NodeInput("scale_in", NodeParam::kVec2, QVector2D(1.0f, 1.0f));
  scale_input_->set_property("min", QVector2D(0, 0));
  scale_input_->set_property("view", "percent");
  scale_input_->set_property("disabley", true);
  AddInput(scale_input_);

  uniform_scale_input_ = new NodeInput("uniform_scale_in", NodeParam::kBoolean, true);
  uniform_scale_input_->set_is_keyframable(false);
  uniform_scale_input_->set_connectable(false);
  connect(uniform_scale_input_, &NodeInput::ValueChanged, this, &MatrixGenerator::UniformScaleChanged);
  AddInput(uniform_scale_input_);

  anchor_input_ = new NodeInput("anchor_in", NodeParam::kVec2, QVector2D());
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

QString MatrixGenerator::ShortName() const
{
  return tr("Ortho");
}

QString MatrixGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.transform");
}

QVector<Node::CategoryID> MatrixGenerator::Category() const
{
  return {kCategoryGenerator, kCategoryMath};
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
  // Push matrix output
  QMatrix4x4 mat = GenerateMatrix(value, true, false, false, false);
  NodeValueTable output = value.Merge();
  output.Push(NodeParam::kMatrix, mat, this);
  return output;
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(NodeValueDatabase &value, bool take, bool ignore_anchor, bool ignore_position, bool ignore_scale) const
{
  QVector2D anchor;
  QVector2D position;
  QVector2D scale;

  if (!ignore_anchor) {
    if (take) {
      // Take and store
      anchor = value[anchor_input_].Take(NodeParam::kVec2).value<QVector2D>();
    } else {
      // Get and store
      anchor = value[anchor_input_].Get(NodeParam::kVec2).value<QVector2D>();
    }
  } else if (take) {
    // Just take
    value[anchor_input_].Take(NodeParam::kVec2).value<QVector2D>();
  }

  if (!ignore_scale) {
    if (take) {
      scale = value[scale_input_].Take(NodeParam::kVec2).value<QVector2D>();
    } else {
      scale = value[scale_input_].Get(NodeParam::kVec2).value<QVector2D>();
    }
  } else if (take) {
    value[scale_input_].Take(NodeParam::kVec2).value<QVector2D>();
  }

  if (!ignore_position) {
    if (take) {
      position = value[position_input_].Take(NodeParam::kVec2).value<QVector2D>();
    } else {
      position = value[position_input_].Get(NodeParam::kVec2).value<QVector2D>();
    }
  } else if (take) {
    value[position_input_].Take(NodeParam::kVec2).value<QVector2D>();
  }

  if (take) {
    return GenerateMatrix(position,
                          value[rotation_input_].Take(NodeParam::kFloat).toFloat(),
                          scale,
                          value[uniform_scale_input_].Take(NodeParam::kBoolean).toBool(),
                          anchor);
  } else {
    return GenerateMatrix(position,
                          value[rotation_input_].Get(NodeParam::kFloat).toFloat(),
                          scale,
                          value[uniform_scale_input_].Get(NodeParam::kBoolean).toBool(),
                          anchor);

  }
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(const QVector2D& pos,
                                           const float& rot,
                                           const QVector2D& scale,
                                           bool uniform_scale,
                                           const QVector2D& anchor)
{
  QMatrix4x4 mat;

  // Position
  mat.translate(pos);

  // Rotation
  mat.rotate(rot, 0, 0, 1);

  // Scale (convert to a QVector3D so that the identity matrix is preserved if all values are 1.0f)
  QVector3D full_scale;
  if (uniform_scale) {
    full_scale = QVector3D(scale.x(), scale.x(), 1.0f);
  } else {
    full_scale = QVector3D(scale, 1.0f);
  }
  mat.scale(full_scale);

  // Anchor Point
  mat.translate(-anchor);

  return mat;
}

void MatrixGenerator::UniformScaleChanged()
{
  scale_input_->set_property("disabley", uniform_scale_input_->get_standard_value().toBool());
}

}

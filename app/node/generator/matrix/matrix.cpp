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

#include "widget/slider/floatslider.h"

namespace olive {

const QString MatrixGenerator::kPositionInput = QStringLiteral("pos_in");
const QString MatrixGenerator::kRotationInput = QStringLiteral("rot_in");
const QString MatrixGenerator::kScaleInput = QStringLiteral("scale_in");
const QString MatrixGenerator::kUniformScaleInput = QStringLiteral("uniform_scale_in");
const QString MatrixGenerator::kAnchorInput = QStringLiteral("anchor_in");

MatrixGenerator::MatrixGenerator()
{
  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0.0, 0.0));

  AddInput(kRotationInput, NodeValue::kFloat, 0.0);

  AddInput(kScaleInput, NodeValue::kVec2, QVector2D(1.0f, 1.0f));
  SetInputProperty(kScaleInput, QStringLiteral("min"), QVector2D(0, 0));
  SetInputProperty(kScaleInput, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(kScaleInput, QStringLiteral("disabley"), true);

  AddInput(kUniformScaleInput, NodeValue::kBoolean, true, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kAnchorInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
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
  return QStringLiteral("org.olivevideoeditor.Olive.ortho");
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
  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kRotationInput, tr("Rotation"));
  SetInputName(kScaleInput, tr("Scale"));
  SetInputName(kUniformScaleInput, tr("Uniform Scale"));
  SetInputName(kAnchorInput, tr("Anchor Point"));
}

NodeValueTable MatrixGenerator::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  // Push matrix output
  QMatrix4x4 mat = GenerateMatrix(value, true, false, false, false);
  NodeValueTable out = value.Merge();
  out.Push(NodeValue::kMatrix, mat, this);
  return out;
}

QMatrix4x4 MatrixGenerator::GenerateMatrix(NodeValueDatabase &value, bool take, bool ignore_anchor, bool ignore_position, bool ignore_scale) const
{
  QVector2D anchor;
  QVector2D position;
  QVector2D scale;

  if (!ignore_anchor) {
    if (take) {
      // Take and store
      anchor = value[kAnchorInput].Take(NodeValue::kVec2).value<QVector2D>();
    } else {
      // Get and store
      anchor = value[kAnchorInput].Get(NodeValue::kVec2).value<QVector2D>();
    }
  } else if (take) {
    // Just take
    value[kAnchorInput].Take(NodeValue::kVec2).value<QVector2D>();
  }

  if (!ignore_scale) {
    if (take) {
      scale = value[kScaleInput].Take(NodeValue::kVec2).value<QVector2D>();
    } else {
      scale = value[kScaleInput].Get(NodeValue::kVec2).value<QVector2D>();
    }
  } else if (take) {
    value[kScaleInput].Take(NodeValue::kVec2).value<QVector2D>();
  }

  if (!ignore_position) {
    if (take) {
      position = value[kPositionInput].Take(NodeValue::kVec2).value<QVector2D>();
    } else {
      position = value[kPositionInput].Get(NodeValue::kVec2).value<QVector2D>();
    }
  } else if (take) {
    value[kPositionInput].Take(NodeValue::kVec2).value<QVector2D>();
  }

  if (take) {
    return GenerateMatrix(position,
                          value[kRotationInput].Take(NodeValue::kFloat).toFloat(),
                          scale,
                          value[kUniformScaleInput].Take(NodeValue::kBoolean).toBool(),
                          anchor);
  } else {
    return GenerateMatrix(position,
                          value[kRotationInput].Get(NodeValue::kFloat).toFloat(),
                          scale,
                          value[kUniformScaleInput].Get(NodeValue::kBoolean).toBool(),
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

void MatrixGenerator::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kUniformScaleInput) {
    SetInputProperty(kScaleInput, QStringLiteral("disabley"), GetStandardValue(kUniformScaleInput).toBool());
  }
}

}

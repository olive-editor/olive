/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "cropdistortnode.h"

#include "common/util.h"
#include "core.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString CropDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString CropDistortNode::kLeftInput = QStringLiteral("left_in");
const QString CropDistortNode::kTopInput = QStringLiteral("top_in");
const QString CropDistortNode::kRightInput = QStringLiteral("right_in");
const QString CropDistortNode::kBottomInput = QStringLiteral("bottom_in");
const QString CropDistortNode::kFeatherInput = QStringLiteral("feather_in");

#define super Node

CropDistortNode::CropDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  CreateCropSideInput(kLeftInput);
  CreateCropSideInput(kTopInput);
  CreateCropSideInput(kRightInput);
  CreateCropSideInput(kBottomInput);

  AddInput(kFeatherInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);

  // Initiate gizmos
  poly_gizmo_ = AddDraggableGizmo<PolygonGizmo>({kLeftInput, kTopInput, kRightInput, kBottomInput});

  point_gizmo_[kGizmoScaleTopLeft] = AddDraggableGizmo<PointGizmo>({kLeftInput, kTopInput});
  point_gizmo_[kGizmoScaleTopCenter] = AddDraggableGizmo<PointGizmo>({kTopInput});
  point_gizmo_[kGizmoScaleTopRight] = AddDraggableGizmo<PointGizmo>({kRightInput, kTopInput});
  point_gizmo_[kGizmoScaleBottomLeft] = AddDraggableGizmo<PointGizmo>({kLeftInput, kBottomInput});
  point_gizmo_[kGizmoScaleBottomCenter] = AddDraggableGizmo<PointGizmo>({kBottomInput});
  point_gizmo_[kGizmoScaleBottomRight] = AddDraggableGizmo<PointGizmo>({kRightInput, kBottomInput});
  point_gizmo_[kGizmoScaleCenterLeft] = AddDraggableGizmo<PointGizmo>({kLeftInput});
  point_gizmo_[kGizmoScaleCenterRight] = AddDraggableGizmo<PointGizmo>({kRightInput});

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void CropDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kLeftInput, tr("Left"));
  SetInputName(kTopInput, tr("Top"));
  SetInputName(kRightInput, tr("Right"));
  SetInputName(kBottomInput, tr("Bottom"));
  SetInputName(kFeatherInput, tr("Feather"));
}

void CropDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.Insert(value);

  if (TexturePtr texture = job.Get(kTextureInput).toTexture()) {
    job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, QVector2D(texture->params().width(), texture->params().height()), this));

    if (!qIsNull(job.Get(kLeftInput).toDouble())
        || !qIsNull(job.Get(kRightInput).toDouble())
        || !qIsNull(job.Get(kTopInput).toDouble())
        || !qIsNull(job.Get(kBottomInput).toDouble())) {
      table->Push(NodeValue::kTexture, texture->toJob(job), this);
    } else {
      table->Push(job.Get(kTextureInput));
    }
  }
}

ShaderCode CropDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/crop.frag")));
}

void CropDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  if (TexturePtr tex = row[kTextureInput].toTexture()) {
    const QVector2D &resolution = tex->virtual_resolution();
    temp_resolution_ = resolution;

    double left_pt = resolution.x() * row[kLeftInput].toDouble();
    double top_pt = resolution.y() * row[kTopInput].toDouble();
    double right_pt = resolution.x() * (1.0 - row[kRightInput].toDouble());
    double bottom_pt = resolution.y() * (1.0 - row[kBottomInput].toDouble());
    double center_x_pt = mid(left_pt, right_pt);
    double center_y_pt = mid(top_pt, bottom_pt);

    point_gizmo_[kGizmoScaleTopLeft]->SetPoint(QPointF(left_pt, top_pt));
    point_gizmo_[kGizmoScaleTopCenter]->SetPoint(QPointF(center_x_pt, top_pt));
    point_gizmo_[kGizmoScaleTopRight]->SetPoint(QPointF(right_pt, top_pt));
    point_gizmo_[kGizmoScaleBottomLeft]->SetPoint(QPointF(left_pt, bottom_pt));
    point_gizmo_[kGizmoScaleBottomCenter]->SetPoint(QPointF(center_x_pt, bottom_pt));
    point_gizmo_[kGizmoScaleBottomRight]->SetPoint(QPointF(right_pt, bottom_pt));
    point_gizmo_[kGizmoScaleCenterLeft]->SetPoint(QPointF(left_pt, center_y_pt));
    point_gizmo_[kGizmoScaleCenterRight]->SetPoint(QPointF(right_pt, center_y_pt));

    poly_gizmo_->SetPolygon(QRectF(left_pt, top_pt, right_pt - left_pt, bottom_pt - top_pt));
  }
}

void CropDistortNode::GizmoDragMove(double x_diff, double y_diff, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  QVector2D res = temp_resolution_;
  x_diff /= res.x();
  y_diff /= res.y();

  for (int j=0; j<gizmo->GetDraggers().size(); j++) {
    NodeInputDragger& i = gizmo->GetDraggers()[j];
    double s = i.GetStartValue().toDouble();
    if (i.GetInput().input().input() == kLeftInput) {
      i.Drag(s + x_diff);
    } else if (i.GetInput().input().input() == kTopInput) {
      i.Drag(s + y_diff);
    } else if (i.GetInput().input().input() == kRightInput) {
      i.Drag(s - x_diff);
    } else if (i.GetInput().input().input() == kBottomInput) {
      i.Drag(s - y_diff);
    }
  }
}

void CropDistortNode::CreateCropSideInput(const QString &id)
{
  AddInput(id, NodeValue::kFloat, 0.0);
  SetInputProperty(id, QStringLiteral("min"), 0.0);
  SetInputProperty(id, QStringLiteral("max"), 1.0);
  SetInputProperty(id, QStringLiteral("view"), FloatSlider::kPercentage);
}

}

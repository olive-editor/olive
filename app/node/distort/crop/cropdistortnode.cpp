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

#include "cropdistortnode.h"

#include "common/lerp.h"
#include "core.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString CropDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString CropDistortNode::kLeftInput = QStringLiteral("left_in");
const QString CropDistortNode::kTopInput = QStringLiteral("top_in");
const QString CropDistortNode::kRightInput = QStringLiteral("right_in");
const QString CropDistortNode::kBottomInput = QStringLiteral("bottom_in");
const QString CropDistortNode::kFeatherInput = QStringLiteral("feather_in");

CropDistortNode::CropDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  CreateCropSideInput(kLeftInput);
  CreateCropSideInput(kTopInput);
  CreateCropSideInput(kRightInput);
  CreateCropSideInput(kBottomInput);

  AddInput(kFeatherInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kFeatherInput, QStringLiteral("min"), 0.0);
}

void CropDistortNode::Retranslate()
{
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
  job.InsertValue(value);
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  if (!job.GetValue(kTextureInput).data().isNull()) {
    if (!qIsNull(job.GetValue(kLeftInput).data().toDouble())
        || !qIsNull(job.GetValue(kRightInput).data().toDouble())
        || !qIsNull(job.GetValue(kTopInput).data().toDouble())
        || !qIsNull(job.GetValue(kBottomInput).data().toDouble())) {
      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table->Push(NodeValue::kTexture, job.GetValue(kTextureInput).data(), this);
    }
  }
}

ShaderCode CropDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/crop.frag")));
}

void CropDistortNode::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  const QVector2D &resolution = globals.resolution();

  const double handle_radius = GetGizmoHandleRadius(p->transform());

  p->setPen(QPen(Qt::white, 0));

  double left_pt = resolution.x() * row[kLeftInput].data().toDouble();
  double top_pt = resolution.y() * row[kTopInput].data().toDouble();
  double right_pt = resolution.x() * (1.0 - row[kRightInput].data().toDouble());
  double bottom_pt = resolution.y() * (1.0 - row[kBottomInput].data().toDouble());
  double center_x_pt = lerp(left_pt, right_pt, 0.5);
  double center_y_pt = lerp(top_pt, bottom_pt, 0.5);

  gizmo_whole_rect_ = QRectF(left_pt, top_pt, right_pt - left_pt, bottom_pt - top_pt);
  p->drawRect(gizmo_whole_rect_);

  gizmo_resize_handle_[kGizmoScaleTopLeft] = CreateGizmoHandleRect(QPointF(left_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleTopCenter] = CreateGizmoHandleRect(QPointF(center_x_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleTopRight] = CreateGizmoHandleRect(QPointF(right_pt, top_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomLeft] = CreateGizmoHandleRect(QPointF(left_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomCenter] = CreateGizmoHandleRect(QPointF(center_x_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleBottomRight] = CreateGizmoHandleRect(QPointF(right_pt, bottom_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleCenterLeft] = CreateGizmoHandleRect(QPointF(left_pt, center_y_pt), handle_radius);
  gizmo_resize_handle_[kGizmoScaleCenterRight] = CreateGizmoHandleRect(QPointF(right_pt, center_y_pt), handle_radius);

  p->setPen(Qt::NoPen);
  p->setBrush(Qt::white);
  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_resize_handle_, kGizmoScaleCount);
}

bool CropDistortNode::GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p)
{
  bool found_handle = false;

  bool gizmo_active[kGizmoScaleCount] = {false};

  for (int i=0; i<kGizmoScaleCount; i++) {
    gizmo_active[i] = gizmo_resize_handle_[i].contains(p);

    if (gizmo_active[i]) {
      found_handle = true;
      break;
    }
  }

  bool in_rect = found_handle ? false : gizmo_whole_rect_.contains(p);

  gizmo_drag_ = kGizmoNone;

  // Drag handle
  if (gizmo_active[kGizmoScaleTopLeft]
      || gizmo_active[kGizmoScaleCenterLeft]
      || gizmo_active[kGizmoScaleBottomLeft]
      || in_rect) {
    gizmo_drag_ |= kGizmoLeft;

    gizmo_start_.append(row[kLeftInput].data());
  }

  if (gizmo_active[kGizmoScaleTopLeft]
      || gizmo_active[kGizmoScaleTopCenter]
      || gizmo_active[kGizmoScaleTopRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoTop;

    gizmo_start_.append(row[kTopInput].data());
  }

  if (gizmo_active[kGizmoScaleTopRight]
      || gizmo_active[kGizmoScaleCenterRight]
      || gizmo_active[kGizmoScaleBottomRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoRight;

    gizmo_start_.append(row[kRightInput].data());
  }

  if (gizmo_active[kGizmoScaleBottomLeft]
      || gizmo_active[kGizmoScaleBottomCenter]
      || gizmo_active[kGizmoScaleBottomRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoBottom;

    gizmo_start_.append(row[kBottomInput].data());
  }

  if (gizmo_drag_ > kGizmoNone) {
    gizmo_res_ = globals.resolution();
    gizmo_drag_start_ = p;

    return true;
  }

  return false;
}

void CropDistortNode::GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers)
{
  if (gizmo_dragger_.isEmpty()) {
    gizmo_dragger_.resize(gizmo_start_.size());

    int counter = 0;

    if (gizmo_drag_ & kGizmoLeft) {
      gizmo_dragger_[counter].Start(NodeInput(this, kLeftInput), time);
      counter++;
    }

    if (gizmo_drag_ & kGizmoTop) {
      gizmo_dragger_[counter].Start(NodeInput(this, kTopInput), time);
      counter++;
    }

    if (gizmo_drag_ & kGizmoRight) {
      gizmo_dragger_[counter].Start(NodeInput(this, kRightInput), time);
      counter++;
    }

    if (gizmo_drag_ & kGizmoBottom) {
      gizmo_dragger_[counter].Start(NodeInput(this, kBottomInput), time);
      counter++;
    }
  }

  int counter = 0;

  double x_diff = (p.x() - gizmo_drag_start_.x()) / gizmo_res_.x();
  double y_diff = (p.y() - gizmo_drag_start_.y()) / gizmo_res_.y();

  if (gizmo_drag_ & kGizmoLeft) {
    gizmo_dragger_[counter].Drag(gizmo_start_[counter].toDouble() + x_diff);
    counter++;
  }

  if (gizmo_drag_ & kGizmoTop) {
    gizmo_dragger_[counter].Drag(gizmo_start_[counter].toDouble() + y_diff);
    counter++;
  }

  if (gizmo_drag_ & kGizmoRight) {
    gizmo_dragger_[counter].Drag(gizmo_start_[counter].toDouble() - x_diff);
    counter++;
  }

  if (gizmo_drag_ & kGizmoBottom) {
    gizmo_dragger_[counter].Drag(gizmo_start_[counter].toDouble() - y_diff);
    counter++;
  }
}

void CropDistortNode::GizmoRelease(MultiUndoCommand *command)
{
  for (NodeInputDragger& i : gizmo_dragger_) {
    i.End(command);
  }
  gizmo_dragger_.clear();

  gizmo_start_.clear();
}

void CropDistortNode::CreateCropSideInput(const QString &id)
{
  AddInput(id, NodeValue::kFloat, 0.0);
  SetInputProperty(id, QStringLiteral("min"), 0.0);
  SetInputProperty(id, QStringLiteral("max"), 1.0);
  SetInputProperty(id, QStringLiteral("view"), FloatSlider::kPercentage);
}

}

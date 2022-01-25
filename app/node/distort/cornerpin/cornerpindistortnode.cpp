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

#include "cornerpindistortnode.h"

#include "common/lerp.h"
#include "core.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString CornerPinDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString CornerPinDistortNode::kTopLeftInput = QStringLiteral("top_left_in");
const QString CornerPinDistortNode::kTopRightInput = QStringLiteral("top_right_in");
const QString CornerPinDistortNode::kBottomRightInput = QStringLiteral("bottom_right_in");
const QString CornerPinDistortNode::kBottomLeftInput = QStringLiteral("bottom_left_in");

CornerPinDistortNode::CornerPinDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
  AddInput(kTopLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kTopRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomRightInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
  AddInput(kBottomLeftInput, NodeValue::kVec2, QVector2D(0.0, 0.0));
}

void CornerPinDistortNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kTopLeftInput, tr("Top Left"));
  SetInputName(kTopRightInput, tr("Top Right"));
  SetInputName(kBottomRightInput, tr("Bottom Right"));
  SetInputName(kBottomLeftInput, tr("Bottom Left"));
}

void CornerPinDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  if (!job.GetValue(kTextureInput).data().isNull()) {
    if (!qIsNull(job.GetValue(kTopLeftInput).data().toDouble())
        || !qIsNull(job.GetValue(kTopRightInput).data().toDouble())
        || !qIsNull(job.GetValue(kBottomLeftInput).data().toDouble())
        || !qIsNull(job.GetValue(kBottomRightInput).data().toDouble())) {
      table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table->Push(NodeValue::kTexture, job.GetValue(kTextureInput).data(), this);
    }
  }
}

ShaderCode CornerPinDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  // Need vertex as well
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.frag")));
}

void CornerPinDistortNode::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  const QVector2D &resolution = globals.resolution();

  const double handle_radius = GetGizmoHandleRadius(p->transform());

  p->setPen(QPen(Qt::white, 0));

  QPointF top_left = QPointF(row[kTopLeftInput].data().value<QVector2D>().x(),
                             row[kTopLeftInput].data().value<QVector2D>().y());

  QPointF top_right = QPointF(resolution.x() + row[kTopRightInput].data().value<QVector2D>().x(),
                              row[kTopRightInput].data().value<QVector2D>().y());

  QPointF bottom_right = QPointF(resolution.x() + row[kBottomRightInput].data().value<QVector2D>().x(),
                                 resolution.y() + row[kBottomRightInput].data().value<QVector2D>().y());
  QPointF bottom_left = QPointF(row[kBottomLeftInput].data().value<QVector2D>().x(),
                                resolution.y() + row[kBottomLeftInput].data().value<QVector2D>().y());

  SetInputProperty(kTopLeftInput, QStringLiteral("offset"), QVector2D(0.0, 0.0));
  SetInputProperty(kTopRightInput, QStringLiteral("offset"), QVector2D(resolution.x() , 0.0));
  SetInputProperty(kBottomRightInput, QStringLiteral("offset"), resolution);
  SetInputProperty(kBottomLeftInput, QStringLiteral("offset"), QVector2D(0.0, resolution.y()));

  p->drawLine(QLineF(top_left, top_right));
  p->drawLine(QLineF(top_right, bottom_right));
  p->drawLine(QLineF(bottom_right, bottom_left));
  p->drawLine(QLineF(bottom_left, top_left));

  gizmo_resize_handle_[0] = CreateGizmoHandleRect(top_left, handle_radius);
  gizmo_resize_handle_[1] = CreateGizmoHandleRect(top_right, handle_radius);
  gizmo_resize_handle_[2] = CreateGizmoHandleRect(bottom_right, handle_radius);
  gizmo_resize_handle_[3] = CreateGizmoHandleRect(bottom_left, handle_radius);

  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_resize_handle_, 4);
}

bool CornerPinDistortNode::GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p)
{
  return false;
}

}

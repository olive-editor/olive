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
  job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, globals.resolution(), this));

  const QVector2D &resolution = globals.resolution();
  QVector2D top_left = QVector2D(ValueToPixel(0, value, resolution)) / (resolution / 2.0) - QVector2D(1.0, 1.0);
  QVector2D top_right = QVector2D(ValueToPixel(1, value, resolution)) / (resolution / 2.0) - QVector2D(1.0, 1.0);
  QVector2D bottom_right = QVector2D(ValueToPixel(2, value, resolution)) / (resolution / 2.0) - QVector2D(1.0, 1.0);
  QVector2D bottom_left = QVector2D(ValueToPixel(3, value, resolution)) / (resolution / 2.0) - QVector2D(1.0, 1.0);

  QVector<float> blit_vertices = {top_left.x(), top_left.y(), 0.0f, //
                                  top_right.x(), top_right.y(), 0.0f,
                                  bottom_right.x(),  bottom_right.y(), 0.0f,

                                  top_left.x(), top_left.y(), 0.0f,
                                  bottom_left.x(),  bottom_left.y(), 0.0f,
                                  bottom_right.x(),  bottom_right.y(), 0.0f};
  job.SetVertexCoordinates(blit_vertices);

  table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
}

ShaderCode CornerPinDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.frag")),//);//,
                    FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/cornerpin.vert")));
}

QPointF CornerPinDistortNode::ValueToPixel(int value, const NodeValueRow& row, const QVector2D &resolution) const
{
  Q_ASSERT(value >= 0 && value <= 3);
  switch (value) {
    case 0:
      return QPointF(row[kTopLeftInput].data().value<QVector2D>().x(),
                     row[kTopLeftInput].data().value<QVector2D>().y());
      break;
    case 1:
      return QPointF(resolution.x() + row[kTopRightInput].data().value<QVector2D>().x(),
                     row[kTopRightInput].data().value<QVector2D>().y());
      break;
    case 2:
      return QPointF(resolution.x() + row[kBottomRightInput].data().value<QVector2D>().x(),
                     resolution.y() + row[kBottomRightInput].data().value<QVector2D>().y());
      break;
    case 3:
      return QPointF(row[kBottomLeftInput].data().value<QVector2D>().x(),
                     row[kBottomLeftInput].data().value<QVector2D>().y() + resolution.y());
      break;
    default:
      return QPointF(); // should never happen
  }
}

void CornerPinDistortNode::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  const QVector2D &resolution = globals.resolution();

  const double handle_radius = GetGizmoHandleRadius(p->transform());

  p->setPen(QPen(Qt::white, 0));

  QPointF top_left = ValueToPixel(0, row, resolution);

  QPointF top_right = ValueToPixel(1, row, resolution);

  QPointF bottom_right = ValueToPixel(2, row, resolution);

  QPointF bottom_left = ValueToPixel(3, row, resolution);

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

  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_resize_handle_, kGizmoCornerCount);


  float m1 = (top_right.y() - bottom_left.y()) / (top_right.x() - bottom_left.x());
  float c1 = bottom_left.y() - m1 * bottom_left.x();
  float m2 = (bottom_right.y() - top_left.y()) / (bottom_right.x() - top_left.x());
  float c2 = top_left.y() - m2 * top_left.x();
  float mid_x = (c2 - c1) / (m1 - m2);
  float mid_y = m1 * mid_x + c1;

  float d0 = QVector2D(mid_x - bottom_left.x(), mid_y - bottom_left.y()).length();
  float d1 = QVector2D(bottom_right.x() - mid_x, mid_y - bottom_right.y()).length();
  float d2 = QVector2D(top_right.x() - mid_x, top_right.y() - mid_y).length();
  float d3 = QVector2D(mid_x - top_left.x(), top_left.y() - mid_y).length();
  /*
  qDebug() << "Vert 0" << (d1 + d3) / d3;
  qDebug() << "Vert 1" << (d0 + d2) / d2;
  qDebug() << "Vert 2" << (d3 + d1) / d1;
  qDebug() << "Vert 3" << (d2 + d0) / d0;
  qDebug() << "=========";
  */
}

bool CornerPinDistortNode::GizmoPress(const NodeValueRow &row, const NodeGlobals &globals, const QPointF &p)
{
  bool gizmo_active[kGizmoCornerCount] = {false};

  for (int i = 0; i < kGizmoCornerCount; i++) {
    gizmo_active[i] = gizmo_resize_handle_[i].contains(p);

    if (gizmo_active[i]) {
      gizmo_drag_start_ = p;
      gizmo_res_ = globals.resolution();
      gizmo_drag_ = i;

      switch (i) {
        case 0:
          gizmo_start_ = row[kTopLeftInput].data();
          break;
        case 1:
          gizmo_start_ = row[kTopRightInput].data();
          break;
        case 2:
          gizmo_start_ = row[kBottomRightInput].data();
          break;
        case 3:
          gizmo_start_ = row[kBottomLeftInput].data();
          break;
      }
      return true;
    }
  }

  return false;
}

void CornerPinDistortNode::GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers)
{
  if (gizmo_dragger_.isEmpty()) {
    gizmo_dragger_.resize(2);
    if (gizmo_drag_ == 0) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kTopLeftInput), 1), time);
    }
    if (gizmo_drag_ == 1) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kTopRightInput), 1), time);
    }
    if (gizmo_drag_ == 2) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomRightInput), 1), time);
    }
    if (gizmo_drag_ == 3) {
      gizmo_dragger_[0].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 0), time);
      gizmo_dragger_[1].Start(NodeKeyframeTrackReference(NodeInput(this, kBottomLeftInput), 1), time);
    }
  }

  double x_diff = (p.x() - gizmo_drag_start_.x()); // / gizmo_res_.x();
  double y_diff = (p.y() - gizmo_drag_start_.y());// / gizmo_res_.y();

  QVector2D move = QVector2D(gizmo_start_.value<QVector2D>().x() + x_diff, gizmo_start_.value<QVector2D>().y() + y_diff);

  gizmo_dragger_[0].Drag(move.x());
  gizmo_dragger_[1].Drag(move.y());

}

void CornerPinDistortNode::GizmoRelease(MultiUndoCommand *command) {
  for (NodeInputDragger &i : gizmo_dragger_) {
    i.End(command);
  }
  gizmo_dragger_.clear();

  gizmo_start_.clear();
}

}

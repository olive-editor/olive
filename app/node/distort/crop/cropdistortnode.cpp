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

#include "cropdistortnode.h"

#include "common/lerp.h"

namespace olive {

CropDistortNode::CropDistortNode()
{
  texture_input_ = new NodeInput(QStringLiteral("tex_in"), NodeParam::kTexture);
  AddInput(texture_input_);

  left_input_ = new NodeInput(QStringLiteral("left_in"), NodeParam::kFloat, 0.0);
  left_input_->set_property(QStringLiteral("min"), 0.0);
  left_input_->set_property(QStringLiteral("max"), 1.0);
  left_input_->set_property(QStringLiteral("view"), QStringLiteral("percent"));
  AddInput(left_input_);

  top_input_ = new NodeInput(QStringLiteral("top_in"), NodeParam::kFloat, 0.0);
  top_input_->set_property(QStringLiteral("min"), 0.0);
  top_input_->set_property(QStringLiteral("max"), 1.0);
  top_input_->set_property(QStringLiteral("view"), QStringLiteral("percent"));
  AddInput(top_input_);

  right_input_ = new NodeInput(QStringLiteral("right_in"), NodeParam::kFloat, 0.0);
  right_input_->set_property(QStringLiteral("min"), 0.0);
  right_input_->set_property(QStringLiteral("max"), 1.0);
  right_input_->set_property(QStringLiteral("view"), QStringLiteral("percent"));
  AddInput(right_input_);

  bottom_input_ = new NodeInput(QStringLiteral("bottom_in"), NodeParam::kFloat, 0.0);
  bottom_input_->set_property(QStringLiteral("min"), 0.0);
  bottom_input_->set_property(QStringLiteral("max"), 1.0);
  bottom_input_->set_property(QStringLiteral("view"), QStringLiteral("percent"));
  AddInput(bottom_input_);

  feather_input_ = new NodeInput(QStringLiteral("feather_in"), NodeParam::kFloat, 0.0);
  feather_input_->set_property(QStringLiteral("min"), 0.0);
  AddInput(feather_input_);
}

void CropDistortNode::Retranslate()
{
  texture_input_->set_name(tr("Texture"));
  left_input_->set_name(tr("Left"));
  top_input_->set_name(tr("Top"));
  right_input_->set_name(tr("Right"));
  bottom_input_->set_name(tr("Bottom"));
  feather_input_->set_name(tr("Feather"));
}

NodeValueTable CropDistortNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;
  job.InsertValue(texture_input_, value);
  job.InsertValue(left_input_, value);
  job.InsertValue(top_input_, value);
  job.InsertValue(right_input_, value);
  job.InsertValue(bottom_input_, value);
  job.InsertValue(feather_input_, value);
  job.InsertValue(QStringLiteral("resolution_in"),
                  ShaderValue(value[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")), NodeParam::kVec2));
  job.SetAlphaChannelRequired(true);

  NodeValueTable table = value.Merge();

  if (!job.GetValue(texture_input_).data.isNull()) {
    if (!qIsNull(job.GetValue(left_input_).data.toDouble())
        || !qIsNull(job.GetValue(right_input_).data.toDouble())
        || !qIsNull(job.GetValue(top_input_).data.toDouble())
        || !qIsNull(job.GetValue(bottom_input_).data.toDouble())) {
      table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(NodeParam::kTexture, job.GetValue(texture_input_).data, this);
    }
  }

  return table;
}

ShaderCode CropDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/crop.frag")));
}

void CropDistortNode::DrawGizmos(NodeValueDatabase &db, QPainter *p)
{
  QVector2D resolution = db[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")).value<QVector2D>();

  const int handle_radius = GetGizmoHandleRadius();

  p->setPen(QPen(Qt::white, 0));

  double left_pt = resolution.x() * db[left_input_].Get(NodeParam::kFloat).toDouble();
  double top_pt = resolution.y() * db[top_input_].Get(NodeParam::kFloat).toDouble();
  double right_pt = resolution.x() * (1.0 - db[right_input_].Get(NodeParam::kFloat).toDouble());
  double bottom_pt = resolution.y() * (1.0 - db[bottom_input_].Get(NodeParam::kFloat).toDouble());
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

bool CropDistortNode::GizmoPress(NodeValueDatabase &db, const QPointF &p)
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

  qDebug() << "Active handles:";
  for (int i=0; i<kGizmoScaleCount; i++) {
    qDebug() << "  " << i << gizmo_active[i];
  }
  qDebug() << "  Rect:" << in_rect;

  gizmo_drag_ = kGizmoNone;

  // Drag handle
  if (gizmo_active[kGizmoScaleTopLeft]
      || gizmo_active[kGizmoScaleCenterLeft]
      || gizmo_active[kGizmoScaleBottomLeft]
      || in_rect) {
    gizmo_drag_ |= kGizmoLeft;

    gizmo_start_.append(db[left_input_].Get(NodeParam::kFloat));
  }

  if (gizmo_active[kGizmoScaleTopLeft]
      || gizmo_active[kGizmoScaleTopCenter]
      || gizmo_active[kGizmoScaleTopRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoTop;

    gizmo_start_.append(db[top_input_].Get(NodeParam::kFloat));
  }

  if (gizmo_active[kGizmoScaleTopRight]
      || gizmo_active[kGizmoScaleCenterRight]
      || gizmo_active[kGizmoScaleBottomRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoRight;

    gizmo_start_.append(db[right_input_].Get(NodeParam::kFloat));
  }

  if (gizmo_active[kGizmoScaleBottomLeft]
      || gizmo_active[kGizmoScaleBottomCenter]
      || gizmo_active[kGizmoScaleBottomRight]
      || in_rect) {
    gizmo_drag_ |= kGizmoBottom;

    gizmo_start_.append(db[bottom_input_].Get(NodeParam::kFloat));
  }

  if (gizmo_drag_ > kGizmoNone) {
    gizmo_res_ = db[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")).value<QVector2D>();
    gizmo_drag_start_ = p;

    return true;
  }

  return false;
}

void CropDistortNode::GizmoMove(const QPointF &p, const rational &time)
{
  if (gizmo_dragger_.isEmpty()) {
    gizmo_dragger_.resize(gizmo_start_.size());

    int counter = 0;

    if (gizmo_drag_ & kGizmoLeft) {
      gizmo_dragger_[counter].Start(left_input_, time, 0);
      counter++;
    }

    if (gizmo_drag_ & kGizmoTop) {
      gizmo_dragger_[counter].Start(top_input_, time, 0);
      counter++;
    }

    if (gizmo_drag_ & kGizmoRight) {
      gizmo_dragger_[counter].Start(right_input_, time, 0);
      counter++;
    }

    if (gizmo_drag_ & kGizmoBottom) {
      gizmo_dragger_[counter].Start(bottom_input_, time, 0);
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

void CropDistortNode::GizmoRelease()
{
  for (NodeInputDragger& i : gizmo_dragger_) {
    i.End();
  }
  gizmo_dragger_.clear();

  gizmo_start_.clear();
}

}

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

#include "transformdistortnode.h"

#include <QGuiApplication>

#include "common/range.h"

namespace olive {

TransformDistortNode::TransformDistortNode()
{
  autoscale_input_ = new NodeInput(QStringLiteral("autoscale_in"), NodeParam::kCombo, 0);
  AddInput(autoscale_input_);

  interpolation_input_ = new NodeInput(QStringLiteral("interpolation_in"), NodeParam::kCombo, 2);
  AddInput(interpolation_input_);

  texture_input_ = new NodeInput(QStringLiteral("tex_in"), NodeParam::kTexture);
  AddInput(texture_input_);
}

void TransformDistortNode::Retranslate()
{
  MatrixGenerator::Retranslate();

  autoscale_input_->set_name(tr("Auto-Scale"));
  texture_input_->set_name(tr("Texture"));
  interpolation_input_->set_name(tr("Interpolation"));

  autoscale_input_->set_combobox_strings({tr("None"), tr("Fit"), tr("Fill"), tr("Stretch")});
  interpolation_input_->set_combobox_strings({tr("Nearest Neighbor"), tr("Bilinear"), tr("Mipmapped Bilinear")});
}

NodeValueTable TransformDistortNode::Value(NodeValueDatabase &value) const
{
  // Generate matrix
  QMatrix4x4 generated_matrix = GenerateMatrix(value, true, false, false, false);

  // Pop texture
  TexturePtr texture = value[texture_input_].Take(NodeParam::kTexture).value<TexturePtr>();

  // Merge table
  NodeValueTable table = value.Merge();

  // If we have a texture, generate a matrix and make it happen
  if (texture) {
    // Adjust our matrix by the resolutions involved
    QVector2D sequence_res = value[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")).value<QVector2D>();
    QVector2D texture_res(texture->params().width() * texture->pixel_aspect_ratio().toDouble(), texture->params().height());
    AutoScaleType autoscale = static_cast<AutoScaleType>(value[autoscale_input_].Get(NodeParam::kCombo).toInt());

    QMatrix4x4 real_matrix = AdjustMatrixByResolutions(generated_matrix,
                                                       sequence_res,
                                                       texture_res,
                                                       autoscale);

    if (real_matrix.isIdentity()) {
      // We don't expect any changes, just push as normal
      table.Push(NodeParam::kTexture, QVariant::fromValue(texture), this);
    } else {
      // The matrix will transform things
      ShaderJob job;
      job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(texture), NodeParam::kTexture));
      job.InsertValue(QStringLiteral("ove_mvpmat"), ShaderValue(real_matrix, NodeParam::kMatrix));
      job.SetInterpolation(QStringLiteral("ove_maintex"), static_cast<Texture::Interpolation>(value[interpolation_input_].Get(NodeParam::kCombo).toInt()));

      // FIXME: This should be optimized, we can use matrix math to determine if this operation will
      //        end up with gaps in the screen that will require an alpha channel.
      job.SetAlphaChannelRequired(true);

      table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
    }
  }

  return table;
}

ShaderCode TransformDistortNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id);

  // Returns default frag and vert shader
  return ShaderCode();
}

bool TransformDistortNode::GizmoPress(NodeValueDatabase &db, const QPointF &p)
{
  // Store cursor position
  gizmo_drag_pos_ = p;

  // Check scaling
  bool gizmo_scale_active[kGizmoScaleCount] = {false};
  bool scaling = false;

  for (int i=0; i<kGizmoScaleCount; i++) {
    gizmo_scale_active[i] = gizmo_resize_handle_[i].contains(p);

    if (gizmo_scale_active[i]) {
      scaling = true;
      break;
    }
  }

  if (scaling) {

    // Dragging scale handle
    gizmo_start_ = {db[scale_input()].Get(NodeParam::kVec2)};
    gizmo_drag_ = scale_input();

    gizmo_scale_uniform_ = db[uniform_scale_input()].Get(NodeParam::kBoolean).toBool();

    if (gizmo_scale_active[kGizmoScaleTopLeft] || gizmo_scale_active[kGizmoScaleTopRight]
        || gizmo_scale_active[kGizmoScaleBottomLeft] || gizmo_scale_active[kGizmoScaleBottomRight]) {
      gizmo_scale_axes_ = kGizmoScaleBoth;
    } else if (gizmo_scale_active[kGizmoScaleCenterLeft] || gizmo_scale_active[kGizmoScaleCenterRight]) {
      gizmo_scale_axes_ = kGizmoScaleXOnly;
    } else {
      gizmo_scale_axes_ = kGizmoScaleYOnly;
    }

    // Store texture size
    QVector2D texture_sz = db[texture_input()].Get(NodeParam::kTexture).value<QVector2D>();
    gizmo_scale_anchor_ = db[anchor_input()].Get(NodeParam::kVec2).value<QVector2D>() + texture_sz/2;

    if (gizmo_scale_active[kGizmoScaleTopRight]
        || gizmo_scale_active[kGizmoScaleBottomRight]
        || gizmo_scale_active[kGizmoScaleCenterRight]) {
      // Right handles, flip X axis
      gizmo_scale_anchor_.setX(texture_sz.x() - gizmo_scale_anchor_.x());
    }

    if (gizmo_scale_active[kGizmoScaleBottomLeft]
        || gizmo_scale_active[kGizmoScaleBottomRight]
        || gizmo_scale_active[kGizmoScaleBottomCenter]) {
      // Bottom handles, flip Y axis
      gizmo_scale_anchor_.setY(texture_sz.y() - gizmo_scale_anchor_.y());
    }

    // Store current matrix
    gizmo_matrix_ = GenerateMatrix(db, false, true, true, true);

    return true;

  } else if (gizmo_anchor_pt_.contains(p)) {

    // Dragging the anchor point specifically
    gizmo_start_ = {db[anchor_input()].Get(NodeParam::kVec2),
                    db[position_input()].Get(NodeParam::kVec2)};
    gizmo_drag_ = anchor_input();

    // Store current matrix
    gizmo_matrix_ = GenerateMatrix(db, false, true, true, false);

    return true;

  } else if (gizmo_rect_.containsPoint(p, Qt::OddEvenFill)) {

    // Dragging the main rectangle
    gizmo_start_ = {db[position_input()].Get(NodeParam::kVec2)};
    gizmo_drag_ = position_input();

    return true;

  } else {

    // Dragging rotation
    gizmo_start_ = {db[rotation_input()].Get(NodeParam::kFloat)};
    gizmo_drag_ = rotation_input();
    gizmo_start_angle_ = qAtan2(gizmo_drag_pos_.y() - gizmo_anchor_pt_.center().y(),
                                gizmo_drag_pos_.x() - gizmo_anchor_pt_.center().x());

    return true;

  }

  return false;
}

void TransformDistortNode::GizmoMove(const QPointF &p, const rational &time)
{
  QPointF movement = (p - gizmo_drag_pos_);
  QVector2D vec_movement(movement);

  if (gizmo_drag_ == anchor_input()) {

    // Dragging the anchor point around
    if (gizmo_dragger_.isEmpty()) {
      gizmo_dragger_.resize(4);
      gizmo_dragger_[0].Start(anchor_input(), time, 0);
      gizmo_dragger_[1].Start(anchor_input(), time, 1);
      gizmo_dragger_[2].Start(position_input(), time, 0);
      gizmo_dragger_[3].Start(position_input(), time, 1);
    }

    QVector2D inverted_movement(gizmo_matrix_.toTransform().inverted().map(movement));
    QVector2D anchor_cont = gizmo_start_[0].value<QVector2D>() + inverted_movement;
    QVector2D position_cont = gizmo_start_[1].value<QVector2D>() + vec_movement;

    gizmo_dragger_[0].Drag(anchor_cont.x());
    gizmo_dragger_[1].Drag(anchor_cont.y());
    gizmo_dragger_[2].Drag(position_cont.x());
    gizmo_dragger_[3].Drag(position_cont.y());

  } else if (gizmo_drag_ == position_input()) {

    // Dragging the main rectangle around
    if (gizmo_dragger_.isEmpty()) {
      gizmo_dragger_.resize(2);
      gizmo_dragger_[0].Start(position_input(), time, 0);
      gizmo_dragger_[1].Start(position_input(), time, 1);
    }

    QVector2D position_cont = gizmo_start_[0].value<QVector2D>() + vec_movement;

    gizmo_dragger_[0].Drag(position_cont.x());
    gizmo_dragger_[1].Drag(position_cont.y());

  } else if (gizmo_drag_ == scale_input()) {

    // Dragging a resize handle
    if (gizmo_dragger_.isEmpty()) {
      if (gizmo_scale_uniform_ || gizmo_scale_axes_ == kGizmoScaleXOnly) {
        gizmo_dragger_.resize(1);
        gizmo_dragger_[0].Start(scale_input(), time, 0);
      } else if (gizmo_scale_axes_ == kGizmoScaleYOnly) {
        gizmo_dragger_.resize(1);
        gizmo_dragger_[0].Start(scale_input(), time, 1);
      } else {
        gizmo_dragger_.resize(2);
        gizmo_dragger_[0].Start(scale_input(), time, 0);
        gizmo_dragger_[1].Start(scale_input(), time, 1);
      }
    }

    QPointF mouse_relative = gizmo_matrix_.toTransform().inverted().map(QPointF(p - gizmo_anchor_pt_.center()));

    double x_scaled_movement = qAbs(mouse_relative.x() / gizmo_scale_anchor_.x());
    double y_scaled_movement = qAbs(mouse_relative.y() / gizmo_scale_anchor_.y());

    switch (gizmo_scale_axes_) {
    case kGizmoScaleXOnly:
      gizmo_dragger_[0].Drag(x_scaled_movement);
      break;
    case kGizmoScaleYOnly:
      gizmo_dragger_[0].Drag(y_scaled_movement);
      break;
    case kGizmoScaleBoth:
      if (gizmo_scale_uniform_) {
        double distance = std::hypot(mouse_relative.x(), mouse_relative.y());
        double texture_diag = std::hypot(gizmo_scale_anchor_.x(), gizmo_scale_anchor_.y());

        gizmo_dragger_[0].Drag(qAbs(distance / texture_diag));
      } else {
        gizmo_dragger_[0].Drag(x_scaled_movement);
        gizmo_dragger_[1].Drag(y_scaled_movement);
      }
      break;
    }

  } else if (gizmo_drag_ == rotation_input()) {

    // Dragging outside the rectangle to rotate
    if (gizmo_dragger_.isEmpty()) {
      gizmo_dragger_.resize(1);
      gizmo_dragger_[0].Start(rotation_input(), time, 0);
    }

    double current_angle = qAtan2(p.y() - gizmo_anchor_pt_.center().y(),
                                  p.x() - gizmo_anchor_pt_.center().x());

    double rotation_difference = (current_angle - gizmo_start_angle_) * 57.2958;

    double rotation_cont = gizmo_start_[0].toDouble() + rotation_difference;

    gizmo_dragger_[0].Drag(rotation_cont);

  }
}

void TransformDistortNode::GizmoRelease()
{
  for (NodeInputDragger& i : gizmo_dragger_) {
    i.End();
  }
  gizmo_dragger_.clear();

  gizmo_start_.clear();

  gizmo_drag_ = nullptr;
}

QMatrix4x4 TransformDistortNode::AdjustMatrixByResolutions(const QMatrix4x4 &mat, const QVector2D &sequence_res, const QVector2D &texture_res, AutoScaleType autoscale_type)
{
  // First, create an identity matrix
  QMatrix4x4 adjusted_matrix;

  // Scale it to a square based on the sequence's resolution
  adjusted_matrix.scale(2.0 / sequence_res.x(), 2.0 / sequence_res.y(), 1.0);

  // Adjust by the matrix we generated earlier
  adjusted_matrix *= mat;

  // Scale back out to texture size (adjusted by pixel aspect)
  adjusted_matrix.scale(texture_res.x() * 0.5, texture_res.y() * 0.5, 1.0);

  // If auto-scale is enabled, fit the texture to the sequence (without cropping)
  if (autoscale_type != kAutoScaleNone) {
    if (autoscale_type == kAutoScaleStretch) {
      adjusted_matrix.scale(sequence_res.x() / texture_res.x(),
                            sequence_res.y() / texture_res.y(),
                            1.0);
    } else {
      double footage_real_ar = texture_res.x() / texture_res.y();
      double sequence_real_ar = sequence_res.x() / sequence_res.y();

      double scale_by_x = sequence_res.x() / texture_res.x();
      double scale_by_y = sequence_res.y() / texture_res.y();
      double autoscale_val;

      if ((autoscale_type == kAutoScaleFit) == (sequence_real_ar > footage_real_ar)) {
        // Scale by height. Either the sequence is wider than the footage or we're using fill and
        // cutting off the sides
        autoscale_val = scale_by_y;
      } else {
        // Scale by width. Either the footage is wider than the sequence or we're using fill and
        // cutting off the top and bottom
        autoscale_val = scale_by_x;
      }

      adjusted_matrix.scale(autoscale_val, autoscale_val, 1.0);
    }
  }

  return adjusted_matrix;
}

QPointF TransformDistortNode::CreateScalePoint(double x, double y, const QPointF &half_res, const QMatrix4x4 &mat)
{
  return mat.map(QPointF(x, y)) + half_res;
}

void TransformDistortNode::DrawGizmos(NodeValueDatabase &db, QPainter *p)
{
  // 0 pen width is always 1px wide despite any transform
  p->setPen(QPen(Qt::white, 0));

  // Get the sequence resolution
  QVector2D sequence_res = db[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")).value<QVector2D>();
  QVector2D sequence_half_res = sequence_res/2;
  QPointF sequence_half_res_pt = sequence_half_res.toPointF();

  // GizmoTraverser just returns the sizes of the textures and no other data
  QVector2D tex_sz = db[texture_input_].Get(NodeParam::kTexture).value<QVector2D>();

  // Retrieve autoscale value
  AutoScaleType autoscale = static_cast<AutoScaleType>(db[autoscale_input_].Get(NodeParam::kCombo).toInt());

  // Fold values into a matrix for the rectangle
  QMatrix4x4 rectangle_matrix;
  rectangle_matrix.scale(sequence_half_res);
  rectangle_matrix *= AdjustMatrixByResolutions(GenerateMatrix(db, false, false, false, false),
                                                sequence_res,
                                                tex_sz,
                                                autoscale);

  // Create rect and transform it
  const QVector<QPointF> points = {QPointF(-1, -1),
                                   QPointF(1, -1),
                                   QPointF(1, 1),
                                   QPointF(-1, 1),
                                   QPointF(-1, -1)};
  QTransform rectangle_transform = rectangle_matrix.toTransform();
  gizmo_rect_ = rectangle_transform.map(points);
  gizmo_rect_.translate(sequence_half_res_pt);

  // Draw rectangle
  p->drawPolyline(gizmo_rect_);

  // Get handle size (relative to screen space rather than buffer space)
  const double resize_handle_rad = GetGizmoHandleRadius(p->transform());

  // Draw anchor point
  QMatrix4x4 anchor_matrix;
  anchor_matrix.scale(sequence_half_res);
  anchor_matrix *= AdjustMatrixByResolutions(GenerateMatrix(db, false, true, false, false),
                                             sequence_res,
                                             tex_sz,
                                             autoscale);
  QPointF anchor_pt = anchor_matrix.toTransform().map(QPointF(0, 0)) + sequence_half_res_pt;
  const double anchor_pt_radius = resize_handle_rad * 2;

  gizmo_anchor_pt_ = QRectF(anchor_pt.x() - anchor_pt_radius,
                            anchor_pt.y() - anchor_pt_radius,
                            anchor_pt_radius*2,
                            anchor_pt_radius*2);

  p->drawEllipse(anchor_pt, anchor_pt_radius, anchor_pt_radius);

  p->drawLines({QLineF(anchor_pt.x() - anchor_pt_radius, anchor_pt.y(),
                anchor_pt.x() + anchor_pt_radius, anchor_pt.y()),
                QLineF(anchor_pt.x(), anchor_pt.y() - anchor_pt_radius,
                anchor_pt.x(), anchor_pt.y() + anchor_pt_radius)});

  // Draw scale handles
  p->setPen(Qt::NoPen);
  p->setBrush(Qt::white);

  gizmo_resize_handle_[kGizmoScaleTopLeft]      = CreateGizmoHandleRect(CreateScalePoint(-1, -1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleTopCenter]    = CreateGizmoHandleRect(CreateScalePoint( 0, -1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleTopRight]     = CreateGizmoHandleRect(CreateScalePoint( 1, -1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleBottomLeft]   = CreateGizmoHandleRect(CreateScalePoint(-1,  1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleBottomCenter] = CreateGizmoHandleRect(CreateScalePoint( 0,  1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleBottomRight]  = CreateGizmoHandleRect(CreateScalePoint( 1,  1, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleCenterLeft]   = CreateGizmoHandleRect(CreateScalePoint(-1,  0, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);
  gizmo_resize_handle_[kGizmoScaleCenterRight]  = CreateGizmoHandleRect(CreateScalePoint( 1,  0, sequence_half_res_pt, rectangle_matrix), resize_handle_rad);

  DrawAndExpandGizmoHandles(p, resize_handle_rad, gizmo_resize_handle_, kGizmoScaleCount);

  // Use offsets to make the appearance of values that start in the top left, even though we
  // really anchor around the center
  position_input()->set_property(QStringLiteral("offset"), sequence_res * 0.5);
  anchor_input()->set_property(QStringLiteral("offset"), tex_sz * 0.5);
}

}

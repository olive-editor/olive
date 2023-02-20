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

#include "transformdistortnode.h"

#include <QGuiApplication>

namespace olive {

const QString TransformDistortNode::kParentInput = QStringLiteral("parent_in");
const QString TransformDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString TransformDistortNode::kAutoscaleInput = QStringLiteral("autoscale_in");
const QString TransformDistortNode::kInterpolationInput = QStringLiteral("interpolation_in");

#define super MatrixGenerator

TransformDistortNode::TransformDistortNode()
{
  AddInput(kParentInput, NodeValue::kMatrix);

  AddInput(kAutoscaleInput, NodeValue::kCombo, 0);

  AddInput(kInterpolationInput, NodeValue::kCombo, 2);

  PrependInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  // Initiate gizmos
  rotation_gizmo_ = AddDraggableGizmo<ScreenGizmo>();
  rotation_gizmo_->AddInput(NodeInput(this, kRotationInput));
  rotation_gizmo_->SetDragValueBehavior(ScreenGizmo::kAbsolute);

  poly_gizmo_ = AddDraggableGizmo<PolygonGizmo>();
  poly_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0));
  poly_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1));

  anchor_gizmo_ = AddDraggableGizmo<PointGizmo>();
  anchor_gizmo_->SetShape(PointGizmo::kAnchorPoint);
  anchor_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kAnchorInput), 0));
  anchor_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kAnchorInput), 1));
  anchor_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0));
  anchor_gizmo_->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1));

  for (int i=0; i<kGizmoScaleCount; i++) {
    point_gizmo_[i] = AddDraggableGizmo<PointGizmo>();
    point_gizmo_[i]->AddInput(NodeKeyframeTrackReference(NodeInput(this, kScaleInput), 0));
    point_gizmo_[i]->AddInput(NodeKeyframeTrackReference(NodeInput(this, kScaleInput), 1));
    point_gizmo_[i]->SetDragValueBehavior(PointGizmo::kAbsolute);
  }

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

void TransformDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kParentInput, tr("Parent"));
  SetInputName(kAutoscaleInput, tr("Auto-Scale"));
  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kInterpolationInput, tr("Interpolation"));

  SetComboBoxStrings(kAutoscaleInput, {tr("None"), tr("Fit"), tr("Fill"), tr("Stretch")});
  SetComboBoxStrings(kInterpolationInput, {tr("Nearest Neighbor"), tr("Bilinear"), tr("Mipmapped Bilinear")});
}

void TransformDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // Generate matrix
  QMatrix4x4 generated_matrix = GenerateMatrix(value, false, false, false, value[kParentInput].toMatrix());

  // Pop texture
  NodeValue texture_meta = value[kTextureInput];

  TexturePtr job_to_push = nullptr;

  // If we have a texture, generate a matrix and make it happen
  if (TexturePtr texture = texture_meta.toTexture()) {
    // Adjust our matrix by the resolutions involved
    QMatrix4x4 real_matrix = GenerateAutoScaledMatrix(generated_matrix, value, globals, texture->params());

    if (!real_matrix.isIdentity()) {
      // The matrix will transform things
      ShaderJob job;
      job.Insert(QStringLiteral("ove_maintex"), texture_meta);
      job.Insert(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, real_matrix, this));
      job.SetInterpolation(QStringLiteral("ove_maintex"), static_cast<Texture::Interpolation>(value[kInterpolationInput].toInt()));

      // Use global resolution rather than texture resolution because this may result in a size change
      job_to_push = Texture::Job(globals.vparams(), job);
    }
  }

  table->Push(NodeValue::kMatrix, QVariant::fromValue(generated_matrix), this);

  if (!job_to_push) {
    // Re-push whatever value we received
    table->Push(texture_meta);
  } else {
    table->Push(NodeValue::kTexture, job_to_push, this);
  }
}

ShaderCode TransformDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request);

  // Returns default frag and vert shader
  return ShaderCode();
}

void TransformDistortNode::GizmoDragStart(const NodeValueRow &row, double x, double y, const rational &time)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  if (gizmo == anchor_gizmo_) {

    gizmo_inverted_transform_ = GenerateMatrix(row, true, true, false, row[kParentInput].toMatrix()).toTransform().inverted();

  } else if (IsAScaleGizmo(gizmo)) {

    // Dragging scale handle
    TexturePtr tex = row[kTextureInput].toTexture();
    if (!tex) {
      return;
    }

    gizmo_scale_uniform_ = row[kUniformScaleInput].toBool();
    gizmo_anchor_pt_ = (row[kAnchorInput].toVec2() + gizmo->GetGlobals().nonsquare_resolution()/2).toPointF();

    if (gizmo == point_gizmo_[kGizmoScaleTopLeft] || gizmo == point_gizmo_[kGizmoScaleTopRight]
        || gizmo == point_gizmo_[kGizmoScaleBottomLeft] || gizmo == point_gizmo_[kGizmoScaleBottomRight]) {
      gizmo_scale_axes_ = kGizmoScaleBoth;
    } else if (gizmo == point_gizmo_[kGizmoScaleCenterLeft] || gizmo == point_gizmo_[kGizmoScaleCenterRight]) {
      gizmo_scale_axes_ = kGizmoScaleXOnly;
    } else {
      gizmo_scale_axes_ = kGizmoScaleYOnly;
    }

    // Store texture size
    VideoParams texture_params = tex->params();
    QVector2D texture_sz(texture_params.square_pixel_width(), texture_params.height());
    gizmo_scale_anchor_ = row[kAnchorInput].toVec2() + texture_sz/2;

    if (gizmo == point_gizmo_[kGizmoScaleTopRight]
        || gizmo == point_gizmo_[kGizmoScaleBottomRight]
        || gizmo == point_gizmo_[kGizmoScaleCenterRight]) {
      // Right handles, flip X axis
      gizmo_scale_anchor_.setX(texture_sz.x() - gizmo_scale_anchor_.x());
    }

    if (gizmo == point_gizmo_[kGizmoScaleBottomLeft]
        || gizmo == point_gizmo_[kGizmoScaleBottomRight]
        || gizmo == point_gizmo_[kGizmoScaleBottomCenter]) {
      // Bottom handles, flip Y axis
      gizmo_scale_anchor_.setY(texture_sz.y() - gizmo_scale_anchor_.y());
    }

    // Store current matrix
    gizmo_inverted_transform_ = GenerateMatrix(row, true, true, true, row[kParentInput].toMatrix()).toTransform().inverted();

  } else if (gizmo == rotation_gizmo_) {

    gizmo_anchor_pt_ = (row[kAnchorInput].toVec2() + gizmo->GetGlobals().nonsquare_resolution()/2).toPointF();
    gizmo_start_angle_ = std::atan2(y - gizmo_anchor_pt_.y(), x - gizmo_anchor_pt_.x());
    gizmo_last_angle_ = gizmo_start_angle_;
    gizmo_last_alt_angle_ = std::atan2(x - gizmo_anchor_pt_.x(), y - gizmo_anchor_pt_.y());
    gizmo_rotate_wrap_ = 0;
    gizmo_rotate_last_dir_ = kDirectionNone;

  }
}

void TransformDistortNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  if (gizmo == poly_gizmo_) {

    NodeInputDragger &x_drag = gizmo->GetDraggers()[0];
    NodeInputDragger &y_drag = gizmo->GetDraggers()[1];

    x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
    y_drag.Drag(y_drag.GetStartValue().toDouble() + y);

  } else if (gizmo == anchor_gizmo_) {

    NodeInputDragger &x_anchor_drag = gizmo->GetDraggers()[0];
    NodeInputDragger &y_anchor_drag = gizmo->GetDraggers()[1];
    NodeInputDragger &x_pos_drag = gizmo->GetDraggers()[2];
    NodeInputDragger &y_pos_drag = gizmo->GetDraggers()[3];

    QPointF inverted_movement(gizmo_inverted_transform_.map(QPointF(x, y)));

    x_anchor_drag.Drag(x_anchor_drag.GetStartValue().toDouble() + inverted_movement.x());
    y_anchor_drag.Drag(y_anchor_drag.GetStartValue().toDouble() + inverted_movement.y());
    x_pos_drag.Drag(x_pos_drag.GetStartValue().toDouble() + x);
    y_pos_drag.Drag(y_pos_drag.GetStartValue().toDouble() + y);

  } else if (gizmo == rotation_gizmo_) {

    double raw_angle = std::atan2(y - gizmo_anchor_pt_.y(), x - gizmo_anchor_pt_.x());
    double alt_angle = std::atan2(x - gizmo_anchor_pt_.x(), y - gizmo_anchor_pt_.y());

    double current_angle = raw_angle;

    // Detect rotation wrap around
    RotationDirection this_dir = GetDirectionFromAngles(gizmo_last_angle_, raw_angle);
    RotationDirection alt_dir = GetDirectionFromAngles(gizmo_last_alt_angle_, alt_angle);

    if (gizmo_rotate_last_dir_ != kDirectionNone && this_dir != gizmo_rotate_last_dir_) {
      if (alt_dir == gizmo_rotate_last_alt_dir_) {
        if ((raw_angle - gizmo_last_angle_) < 0) {
          gizmo_rotate_wrap_++;
        } else {
          gizmo_rotate_wrap_--;
        }

        this_dir = gizmo_rotate_last_dir_;
        alt_dir = gizmo_rotate_last_alt_dir_;
      }
    }

    gizmo_rotate_last_dir_ = this_dir;
    gizmo_rotate_last_alt_dir_ = alt_dir;
    gizmo_last_angle_ = raw_angle;
    gizmo_last_alt_angle_ = alt_angle;

    current_angle += M_PI*2*gizmo_rotate_wrap_;

    // Convert radians to degrees
    double rotation_difference = (current_angle - gizmo_start_angle_) * 57.2958;

    NodeInputDragger &d = gizmo->GetDraggers()[0];
    d.Drag(d.GetStartValue().toDouble() + rotation_difference);

  } else if (IsAScaleGizmo(gizmo)) {

    QPointF mouse_relative = gizmo_inverted_transform_.map(QPointF(x, y) - gizmo_anchor_pt_);

    double x_scaled_movement = qAbs(mouse_relative.x() / gizmo_scale_anchor_.x());
    double y_scaled_movement = qAbs(mouse_relative.y() / gizmo_scale_anchor_.y());

    NodeInputDragger &x_drag = gizmo->GetDraggers()[0];
    NodeInputDragger &y_drag = gizmo->GetDraggers()[1];

    switch (gizmo_scale_axes_) {
    case kGizmoScaleXOnly:
      x_drag.Drag(x_scaled_movement);
      break;
    case kGizmoScaleYOnly:
      if (gizmo_scale_uniform_) {
        x_drag.Drag(y_scaled_movement);
      } else {
        y_drag.Drag(y_scaled_movement);
      }
      break;
    case kGizmoScaleBoth:
      if (gizmo_scale_uniform_) {
        double distance = std::hypot(mouse_relative.x(), mouse_relative.y());
        double texture_diag = std::hypot(gizmo_scale_anchor_.x(), gizmo_scale_anchor_.y());

        x_drag.Drag(qAbs(distance / texture_diag));
      } else {
        x_drag.Drag(x_scaled_movement);
        y_drag.Drag(y_scaled_movement);
      }
      break;
    }

  }
}

QMatrix4x4 TransformDistortNode::AdjustMatrixByResolutions(const QMatrix4x4 &mat, const QVector2D &sequence_res, const QVector2D &texture_res, const QVector2D &offset, AutoScaleType autoscale_type)
{
  // First, create an identity matrix
  QMatrix4x4 adjusted_matrix;

  // Scale it to a square based on the sequence's resolution
  adjusted_matrix.scale(2.0 / sequence_res.x(), 2.0 / sequence_res.y(), 1.0);

  // Apply offset if applicable
  adjusted_matrix.translate(offset.x(), offset.y());

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

void TransformDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  TexturePtr tex = row[kTextureInput].toTexture();
  if (!tex) {
    return;
  }

  // Get the sequence resolution
  const QVector2D &sequence_res = globals.nonsquare_resolution();
  QVector2D sequence_half_res = sequence_res * 0.5;
  QPointF sequence_half_res_pt = sequence_half_res.toPointF();

  // GizmoTraverser just returns the sizes of the textures and no other data
  VideoParams tex_params = tex->params();
  QVector2D tex_sz(tex_params.square_pixel_width(), tex_params.height());
  QVector2D tex_offset = tex_params.offset();

  // Retrieve autoscale value
  AutoScaleType autoscale = static_cast<AutoScaleType>(row[kAutoscaleInput].toInt());

  // Fold values into a matrix for the rectangle
  QMatrix4x4 rectangle_matrix;
  rectangle_matrix.scale(sequence_half_res.x(), sequence_half_res.y());
  rectangle_matrix *= AdjustMatrixByResolutions(GenerateMatrix(row, false, false, false, row[kParentInput].toMatrix()),
                                                sequence_res,
                                                tex_sz,
                                                tex_offset,
                                                autoscale);

  // Create rect and transform it
  const QVector<QPointF> points = {QPointF(-1, -1),
                                   QPointF(1, -1),
                                   QPointF(1, 1),
                                   QPointF(-1, 1),
                                   QPointF(-1, -1)};
  QTransform rectangle_transform = rectangle_matrix.toTransform();
  QPolygonF r = rectangle_transform.map(points);
  r.translate(sequence_half_res_pt);
  poly_gizmo_->SetPolygon(r);

  // Draw anchor point
  QMatrix4x4 anchor_matrix;
  anchor_matrix.scale(sequence_half_res.x(), sequence_half_res.y());
  anchor_matrix *= AdjustMatrixByResolutions(GenerateMatrix(row, true, false, false, row[kParentInput].toMatrix()),
                                             sequence_res,
                                             tex_sz,
                                             tex_offset,
                                             autoscale);
  anchor_gizmo_->SetPoint(anchor_matrix.toTransform().map(QPointF(0, 0)) + sequence_half_res_pt);

  // Draw scale handles
  point_gizmo_[kGizmoScaleTopLeft]->SetPoint(CreateScalePoint(-1, -1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleTopCenter]->SetPoint(CreateScalePoint( 0, -1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleTopRight]->SetPoint(CreateScalePoint( 1, -1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleBottomLeft]->SetPoint(CreateScalePoint(-1,  1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleBottomCenter]->SetPoint(CreateScalePoint( 0,  1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleBottomRight]->SetPoint(CreateScalePoint( 1,  1, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleCenterLeft]->SetPoint(CreateScalePoint(-1,  0, sequence_half_res_pt, rectangle_matrix));
  point_gizmo_[kGizmoScaleCenterRight]->SetPoint(CreateScalePoint( 1,  0, sequence_half_res_pt, rectangle_matrix));

  // Use offsets to make the appearance of values that start in the top left, even though we
  // really anchor around the center
  SetInputProperty(kPositionInput, QStringLiteral("offset"), sequence_half_res + tex_offset);
  SetInputProperty(kAnchorInput, QStringLiteral("offset"), tex_sz * 0.5);
}

QTransform TransformDistortNode::GizmoTransformation(const NodeValueRow &row, const NodeGlobals &globals) const
{
  if (TexturePtr texture = row[kTextureInput].toTexture()) {
    //auto m = GenerateMatrix(row, false, false, false, row[kParentInput].toMatrix());
    auto m = GenerateMatrix(row, false, false, false, QMatrix4x4());
    return GenerateAutoScaledMatrix(m, row, globals, texture->params()).toTransform();
  }
  return super::GizmoTransformation(row, globals);
}

QPointF TransformDistortNode::CreateScalePoint(double x, double y, const QPointF &half_res, const QMatrix4x4 &mat)
{
  return mat.map(QPointF(x, y)) + half_res;
}

QMatrix4x4 TransformDistortNode::GenerateAutoScaledMatrix(const QMatrix4x4& generated_matrix, const NodeValueRow& value, const NodeGlobals &globals, const VideoParams& texture_params) const
{
  const QVector2D &sequence_res = globals.nonsquare_resolution();
  QVector2D texture_res(texture_params.square_pixel_width(), texture_params.height());
  AutoScaleType autoscale = static_cast<AutoScaleType>(value[kAutoscaleInput].toInt());

  return AdjustMatrixByResolutions(generated_matrix,
                                   sequence_res,
                                   texture_res,
                                   texture_params.offset(),
                                   autoscale);
}

bool TransformDistortNode::IsAScaleGizmo(NodeGizmo *g) const
{
  for (int i=0; i<kGizmoScaleCount; i++) {
    if (point_gizmo_[i] == g) {
      return true;
    }
  }

  return false;
}

TransformDistortNode::RotationDirection TransformDistortNode::GetDirectionFromAngles(double last, double current)
{
  return (current > last) ? kDirectionPositive : kDirectionNegative;
}

}

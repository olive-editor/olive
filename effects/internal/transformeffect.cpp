/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "transformeffect.h"

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QOpenGLFunctions>
#include <QComboBox>
#include <QMouseEvent>

#include "ui/collapsiblewidget.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "project/footage.h"
#include "global/math.h"
#include "ui/labelslider.h"
#include "ui/comboboxex.h"
#include "panels/project.h"
#include "global/debug.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"

TransformEffect::TransformEffect(Clip* c, const EffectMeta* em) : Effect(c, em) {
  SetFlags(Effect::CoordsFlag);

  EffectRow* position_row = new EffectRow(this, tr("Position"));

  position_x = new DoubleField(position_row, "posx"); // position X
  position_y = new DoubleField(position_row, "posy"); // position Y

  EffectRow* scale_row = new EffectRow(this, tr("Scale"));

  // scale X (and Y is uniform scale is selected)
  scale_x = new DoubleField(scale_row, "scalex");
  scale_x->SetMinimum(0);

  // scale Y (disabled if uniform scale is selected)
  scale_y = new DoubleField(scale_row, "scaley");
  scale_y->SetMinimum(0);

  EffectRow* uniform_scale_row = new EffectRow(this, tr("Uniform Scale"));

  uniform_scale_field = new BoolField(uniform_scale_row, "uniformscale"); // uniform scale option

  EffectRow* rotation_row = new EffectRow(this, tr("Rotation"));

  rotation = new DoubleField(rotation_row, "rotation");

  EffectRow* anchor_point_row = new EffectRow(this, tr("Anchor Point"));

  anchor_x_box = new DoubleField(anchor_point_row, "anchorx"); // anchor point X
  anchor_y_box = new DoubleField(anchor_point_row, "anchory"); // anchor point Y

  EffectRow* opacity_row = new EffectRow(this, tr("Opacity"));

  // opacity
  opacity = new DoubleField(opacity_row, "opacity");
  opacity->SetMinimum(0);
  opacity->SetMaximum(100);

  EffectRow* blend_mode_row = new EffectRow(this, tr("Blend Mode"));

  // blend mode
  blend_mode_box = new ComboField(blend_mode_row, "blendmode");
  blend_mode_box->SetColumnSpan(2);
  blend_mode_box->AddItem(tr("Normal"), -1);

  // set up gizmos
  top_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_left_gizmo->set_cursor(Qt::SizeFDiagCursor);
  top_left_gizmo->x_field1 = scale_x;

  top_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_center_gizmo->set_cursor(Qt::SizeVerCursor);
  top_center_gizmo->y_field1 = scale_x;

  top_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_right_gizmo->set_cursor(Qt::SizeBDiagCursor);
  top_right_gizmo->x_field1 = scale_x;

  bottom_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_left_gizmo->set_cursor(Qt::SizeBDiagCursor);
  bottom_left_gizmo->x_field1 = scale_x;

  bottom_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_center_gizmo->set_cursor(Qt::SizeVerCursor);
  bottom_center_gizmo->y_field1 = scale_x;

  bottom_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_right_gizmo->set_cursor(Qt::SizeFDiagCursor);
  bottom_right_gizmo->x_field1 = scale_x;

  left_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  left_center_gizmo->set_cursor(Qt::SizeHorCursor);
  left_center_gizmo->x_field1 = scale_x;

  right_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  right_center_gizmo->set_cursor(Qt::SizeHorCursor);
  right_center_gizmo->x_field1 = scale_x;

  anchor_gizmo = add_gizmo(GIZMO_TYPE_TARGET);
  anchor_gizmo->set_cursor(Qt::SizeAllCursor);
  anchor_gizmo->x_field1 = anchor_x_box;
  anchor_gizmo->y_field1 = anchor_y_box;
  anchor_gizmo->x_field2 = position_x;
  anchor_gizmo->y_field2 = position_y;

  rotate_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  rotate_gizmo->color = Qt::green;
  rotate_gizmo->set_cursor(Qt::SizeAllCursor);
  rotate_gizmo->x_field1 = rotation;

  rect_gizmo = add_gizmo(GIZMO_TYPE_POLY);
  rect_gizmo->x_field1 = position_x;
  rect_gizmo->y_field1 = position_y;

  connect(uniform_scale_field, SIGNAL(Toggled(bool)), this, SLOT(toggle_uniform_scale(bool)));

  // set defaults
  uniform_scale_field->SetValueAt(0, true);
  blend_mode_box->SetValueAt(0, -1);
  anchor_x_box->SetDefault(0);
  anchor_y_box->SetDefault(0);
  opacity->SetDefault(100);
  scale_x->SetDefault(100);
  scale_y->SetDefault(100);

  refresh();
}

void TransformEffect::refresh() {
  if (parent_clip != nullptr && parent_clip->sequence != nullptr) {

    position_x->SetDefault(parent_clip->sequence->width/2);
    position_y->SetDefault(parent_clip->sequence->height/2);

    double x_percent_multipler = 200.0 / parent_clip->sequence->width;
    double y_percent_multipler = 200.0 / parent_clip->sequence->height;

    top_left_gizmo->x_field_multi1 = -x_percent_multipler;
    top_left_gizmo->y_field_multi1 = -y_percent_multipler;
    top_center_gizmo->y_field_multi1 = -y_percent_multipler;
    top_right_gizmo->x_field_multi1 = x_percent_multipler;
    top_right_gizmo->y_field_multi1 = -y_percent_multipler;
    bottom_left_gizmo->x_field_multi1 = -x_percent_multipler;
    bottom_left_gizmo->y_field_multi1 = y_percent_multipler;
    bottom_center_gizmo->y_field_multi1 = y_percent_multipler;
    bottom_right_gizmo->x_field_multi1 = x_percent_multipler;
    bottom_right_gizmo->y_field_multi1 = y_percent_multipler;
    left_center_gizmo->x_field_multi1 = -x_percent_multipler;
    right_center_gizmo->x_field_multi1 = x_percent_multipler;
    rotate_gizmo->x_field_multi1 = x_percent_multipler;

  }
}

void TransformEffect::toggle_uniform_scale(bool enabled) {
  scale_y->SetEnabled(!enabled);

  top_center_gizmo->y_field1 = enabled ? scale_x : scale_y;
  bottom_center_gizmo->y_field1 = enabled ? scale_x : scale_y;
  top_left_gizmo->y_field1 = enabled ? nullptr : scale_y;
  top_right_gizmo->y_field1 = enabled ? nullptr : scale_y;
  bottom_left_gizmo->y_field1 = enabled ? nullptr : scale_y;
  bottom_right_gizmo->y_field1 = enabled ? nullptr : scale_y;
}

void TransformEffect::process_coords(double timecode, GLTextureCoords& coords, int) {
  // position
  coords.matrix.translate(position_x->GetDoubleAt(timecode)-(parent_clip->sequence->width/2),
                          position_y->GetDoubleAt(timecode)-(parent_clip->sequence->height/2),
                          0);

  // anchor point
  int anchor_x_offset = qRound(anchor_x_box->GetDoubleAt(timecode));
  int anchor_y_offset = qRound(anchor_y_box->GetDoubleAt(timecode));

  coords.vertex_top_left -= QVector3D(anchor_x_offset, anchor_y_offset, 0.0f);
  coords.vertex_top_right -= QVector3D(anchor_x_offset, anchor_y_offset, 0.0f);
  coords.vertex_bottom_left -= QVector3D(anchor_x_offset, anchor_y_offset, 0.0f);
  coords.vertex_bottom_right -= QVector3D(anchor_x_offset, anchor_y_offset, 0.0f);

  // rotation
  coords.matrix.rotate(QQuaternion::fromEulerAngles(0, 0, rotation->GetDoubleAt(timecode)));

  // scale
  double sx = scale_x->GetDoubleAt(timecode)*0.01;
  double sy = (uniform_scale_field->GetBoolAt(timecode)) ? sx : scale_y->GetDoubleAt(timecode)*0.01;
  coords.matrix.scale(sx, sy);

  // blend mode
  coords.blendmode = blend_mode_box->GetValueAt(timecode).toInt();

  // opacity
  coords.opacity *= float(opacity->GetDoubleAt(timecode)*0.01);
}

QVector3D LerpVector3D(const QVector3D& a, const QVector3D& b, float t) {
  return QVector3D(
        float_lerp(a.x(), b.x(), t),
        float_lerp(a.y(), b.y(), t),
        float_lerp(a.z(), b.z(), t)
        );
}

void TransformEffect::gizmo_draw(double, GLTextureCoords& coords) {
  top_left_gizmo->world_pos[0] = coords.vertex_top_left;
  top_right_gizmo->world_pos[0] = coords.vertex_top_right;
  bottom_right_gizmo->world_pos[0] = coords.vertex_bottom_right;
  bottom_left_gizmo->world_pos[0] = coords.vertex_bottom_left;

  top_center_gizmo->world_pos[0] = LerpVector3D(coords.vertex_top_left, coords.vertex_top_right, 0.5);
  right_center_gizmo->world_pos[0] = LerpVector3D(coords.vertex_top_right, coords.vertex_bottom_right, 0.5);
  bottom_center_gizmo->world_pos[0] = LerpVector3D(coords.vertex_bottom_right, coords.vertex_bottom_left, 0.5);
  left_center_gizmo->world_pos[0] = LerpVector3D(coords.vertex_bottom_left, coords.vertex_top_left, 0.5);

  rotate_gizmo->world_pos[0] = QVector3D(
        float_lerp(top_center_gizmo->world_pos[0].x(), bottom_center_gizmo->world_pos[0].x(), 0.5f),
        float_lerp(top_center_gizmo->world_pos[0].y(), bottom_center_gizmo->world_pos[0].y(), -0.1f),
        0.0f
        );

  rect_gizmo->world_pos[0] = coords.vertex_top_left;
  rect_gizmo->world_pos[1] = coords.vertex_top_right;
  rect_gizmo->world_pos[2] = coords.vertex_bottom_right;
  rect_gizmo->world_pos[3] = coords.vertex_bottom_left;
}

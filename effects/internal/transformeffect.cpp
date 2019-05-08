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

TransformEffect::TransformEffect(Clip* c) : OldEffectNode(c) {
  SetFlags(OldEffectNode::CoordsFlag);

  position = new Vec2Input(this, "pos", tr("Position"));

  scale = new Vec2Input(this, "scale", tr("Scale"));
  scale->SetMinimum(0);
  scale->SetDefault(100);

  uniform_scale_field = new BoolInput(this, "uniformscale", tr("Uniform Scale"));
  connect(uniform_scale_field, SIGNAL(Toggled(bool)), scale, SLOT(SetSingleValueMode(bool)));
  uniform_scale_field->SetValueAt(0, true);

  rotation = new DoubleInput(this, "rotation", tr("Rotation"));

  anchor_point = new Vec2Input(this, "anchor", tr("Anchor Point"));
  anchor_point->SetDefault(0);

  // opacity
  opacity = new DoubleInput(this, "opacity", tr("Opacity"));
  opacity->SetMinimum(0);
  opacity->SetMaximum(100);
  opacity->SetDefault(100);

  // TEMP - Create matrix output
  NodeIO* matrix_output = new NodeIO(this, "matrix", "Matrix", false, false);
  matrix_output->SetOutputDataType(olive::nodes::kMatrix);

  // set up gizmos
  top_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_left_gizmo->set_cursor(Qt::SizeFDiagCursor);
  top_left_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  top_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_center_gizmo->set_cursor(Qt::SizeVerCursor);
  top_center_gizmo->y_field1 = static_cast<DoubleField*>(scale->Field(0));

  top_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_right_gizmo->set_cursor(Qt::SizeBDiagCursor);
  top_right_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  bottom_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_left_gizmo->set_cursor(Qt::SizeBDiagCursor);
  bottom_left_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  bottom_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_center_gizmo->set_cursor(Qt::SizeVerCursor);
  bottom_center_gizmo->y_field1 = static_cast<DoubleField*>(scale->Field(0));

  bottom_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_right_gizmo->set_cursor(Qt::SizeFDiagCursor);
  bottom_right_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  left_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  left_center_gizmo->set_cursor(Qt::SizeHorCursor);
  left_center_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  right_center_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  right_center_gizmo->set_cursor(Qt::SizeHorCursor);
  right_center_gizmo->x_field1 = static_cast<DoubleField*>(scale->Field(0));

  anchor_gizmo = add_gizmo(GIZMO_TYPE_TARGET);
  anchor_gizmo->set_cursor(Qt::SizeAllCursor);
  anchor_gizmo->x_field1 = static_cast<DoubleField*>(anchor_point->Field(0));
  anchor_gizmo->y_field1 = static_cast<DoubleField*>(anchor_point->Field(1));
  anchor_gizmo->x_field2 = static_cast<DoubleField*>(position->Field(0));
  anchor_gizmo->y_field2 = static_cast<DoubleField*>(position->Field(1));

  rotate_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  rotate_gizmo->color = Qt::green;
  rotate_gizmo->set_cursor(Qt::SizeAllCursor);
  rotate_gizmo->x_field1 = static_cast<DoubleField*>(rotation->Field(0));

  rect_gizmo = add_gizmo(GIZMO_TYPE_POLY);
  rect_gizmo->x_field1 = static_cast<DoubleField*>(position->Field(0));
  rect_gizmo->y_field1 = static_cast<DoubleField*>(position->Field(1));

  connect(uniform_scale_field, SIGNAL(Toggled(bool)), this, SLOT(toggle_uniform_scale(bool)));

  refresh();
}

QString TransformEffect::name()
{
  return tr("Transform");
}

QString TransformEffect::id()
{
  return "org.olivevideoeditor.Olive.transform";
}

QString TransformEffect::category()
{
  return tr("Distort");
}

QString TransformEffect::description()
{
  return tr("Transform the position, scale, and rotation of this clip.");
}

EffectType TransformEffect::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType TransformEffect::subtype()
{
  return olive::kTypeVideo;
}

OldEffectNodePtr TransformEffect::Create(Clip *c)
{
  return std::make_shared<TransformEffect>(c);
}

void TransformEffect::refresh() {
  if (parent_clip != nullptr && parent_clip->track()->sequence() != nullptr) {

    position->SetDefault({parent_clip->track()->sequence()->width()*0.5,
                          parent_clip->track()->sequence()->height()*0.5});

    double x_percent_multipler = 200.0 / parent_clip->track()->sequence()->width();
    double y_percent_multipler = 200.0 / parent_clip->track()->sequence()->height();

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
  scale->SetSingleValueMode(enabled);

  DoubleField* scale_x = static_cast<DoubleField*>(scale->Field(0));
  DoubleField* scale_y = static_cast<DoubleField*>(scale->Field(1));

  top_center_gizmo->y_field1 = enabled ? scale_x : scale_y;
  bottom_center_gizmo->y_field1 = enabled ? scale_x : scale_y;
  top_left_gizmo->y_field1 = enabled ? nullptr : scale_y;
  top_right_gizmo->y_field1 = enabled ? nullptr : scale_y;
  bottom_left_gizmo->y_field1 = enabled ? nullptr : scale_y;
  bottom_right_gizmo->y_field1 = enabled ? nullptr : scale_y;
}

void TransformEffect::process_coords(double timecode, GLTextureCoords& coords, int) {
  // position
  coords.matrix.translate(position->GetVector2DAt(timecode)
                          - QVector2D(parent_clip->track()->sequence()->width()*0.5f,
                                      parent_clip->track()->sequence()->height()*0.5f));

  // anchor point
  QVector2D anchor_val = anchor_point->GetVector2DAt(timecode);

  coords.vertex_top_left -= anchor_val;
  coords.vertex_top_right -= anchor_val;
  coords.vertex_bottom_left -= anchor_val;
  coords.vertex_bottom_right -= anchor_val;

  // rotation
  coords.matrix.rotate(QQuaternion::fromEulerAngles(0, 0, float(rotation->GetDoubleAt(timecode))));

  // scale
  coords.matrix.scale(scale->GetVector2DAt(timecode)*0.01f);

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

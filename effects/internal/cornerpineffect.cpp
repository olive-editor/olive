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

#include "cornerpineffect.h"

#include "global/path.h"
#include "timeline/clip.h"
#include "global/debug.h"

CornerPinEffect::CornerPinEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  SetFlags(Effect::CoordsFlag | Effect::ShaderFlag);

  EffectRow* top_left = new EffectRow(this, tr("Top Left"));
  top_left_x = new DoubleField(top_left, "topleftx");
  top_left_y = new DoubleField(top_left, "toplefty");

  EffectRow* top_right = new EffectRow(this, tr("Top Right"));
  top_right_x = new DoubleField(top_right, "toprightx");
  top_right_y = new DoubleField(top_right, "toprighty");

  EffectRow* bottom_left = new EffectRow(this, tr("Bottom Left"));
  bottom_left_x = new DoubleField(bottom_left, "bottomleftx");
  bottom_left_y = new DoubleField(bottom_left, "bottomlefty");

  EffectRow* bottom_right = new EffectRow(this, tr("Bottom Right"));
  bottom_right_x = new DoubleField(bottom_right, "bottomrightx");
  bottom_right_y = new DoubleField(bottom_right, "bottomrighty");

  EffectRow* perspective_row = new EffectRow(this, tr("Perspective"));
  perspective = new BoolField(perspective_row, "perspective");
  perspective->SetValueAt(0, true);

  top_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_left_gizmo->x_field1 = top_left_x;
  top_left_gizmo->y_field1 = top_left_y;

  top_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_right_gizmo->x_field1 = top_right_x;
  top_right_gizmo->y_field1 = top_right_y;

  bottom_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_left_gizmo->x_field1 = bottom_left_x;
  bottom_left_gizmo->y_field1 = bottom_left_y;

  bottom_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_right_gizmo->x_field1 = bottom_right_x;
  bottom_right_gizmo->y_field1 = bottom_right_y;

  shader_vert_path_ = "cornerpin.vert";
  shader_frag_path_ = "cornerpin.frag";
}

void CornerPinEffect::process_coords(double timecode, GLTextureCoords &coords, int) {
  coords.vertex_top_left += QVector3D(top_left_x->GetDoubleAt(timecode), top_left_y->GetDoubleAt(timecode), 0.0f);

  coords.vertex_top_right += QVector3D(top_right_x->GetDoubleAt(timecode), top_right_y->GetDoubleAt(timecode), 0.0f);

  coords.vertex_bottom_left += QVector3D(bottom_left_x->GetDoubleAt(timecode), bottom_left_y->GetDoubleAt(timecode), 0.0f);

  coords.vertex_bottom_right += QVector3D(bottom_right_x->GetDoubleAt(timecode), bottom_right_y->GetDoubleAt(timecode), 0.0f);
}

void CornerPinEffect::process_shader(double timecode, GLTextureCoords &coords, int) {
  shader_program_->setUniformValue("p0", coords.vertex_bottom_left.x(), coords.vertex_bottom_left.y());
  shader_program_->setUniformValue("p1", coords.vertex_bottom_right.x(), coords.vertex_bottom_right.y());
  shader_program_->setUniformValue("p2", coords.vertex_top_left.x(), coords.vertex_top_left.y());
  shader_program_->setUniformValue("p3", coords.vertex_top_right.x(), coords.vertex_top_right.y());
  shader_program_->setUniformValue("perspective", perspective->GetBoolAt(timecode));
}

void CornerPinEffect::gizmo_draw(double, GLTextureCoords &coords) {
  top_left_gizmo->world_pos[0] = coords.vertex_top_left;
  top_right_gizmo->world_pos[0] = coords.vertex_top_right;
  bottom_right_gizmo->world_pos[0] = coords.vertex_bottom_right;
  bottom_left_gizmo->world_pos[0] = coords.vertex_bottom_left;
}

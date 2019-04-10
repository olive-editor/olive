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

  top_left = new Vec2Input(this, "topleft", tr("Top Left"));

  top_right = new Vec2Input(this, "topright", tr("Top Right"));

  bottom_left = new Vec2Input(this, "bottomleft", tr("Bottom Left"));

  bottom_right = new Vec2Input(this, "bottomright", tr("Bottom Right"));

  perspective = new BoolInput(this, "perspective", tr("Perspective"));
  perspective->SetValueAt(0, true);

  top_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_left_gizmo->x_field1 = static_cast<DoubleField*>(top_left->Field(0));
  top_left_gizmo->y_field1 = static_cast<DoubleField*>(top_left->Field(1));

  top_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  top_right_gizmo->x_field1 = static_cast<DoubleField*>(top_right->Field(0));
  top_right_gizmo->y_field1 = static_cast<DoubleField*>(top_right->Field(1));

  bottom_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_left_gizmo->x_field1 = static_cast<DoubleField*>(bottom_left->Field(0));
  bottom_left_gizmo->y_field1 = static_cast<DoubleField*>(bottom_left->Field(1));

  bottom_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
  bottom_right_gizmo->x_field1 = static_cast<DoubleField*>(bottom_right->Field(0));
  bottom_right_gizmo->y_field1 = static_cast<DoubleField*>(bottom_right->Field(1));

  shader_vert_path_ = "cornerpin.vert";
  shader_frag_path_ = "cornerpin.frag";
}

void CornerPinEffect::process_coords(double timecode, GLTextureCoords &coords, int) {
  coords.vertex_top_left += top_left->GetVector2DAt(timecode);
  coords.vertex_top_right += top_right->GetVector2DAt(timecode);
  coords.vertex_bottom_left += bottom_left->GetVector2DAt(timecode);
  coords.vertex_bottom_right += bottom_right->GetVector2DAt(timecode);
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

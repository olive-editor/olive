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
  SetFlags(Effect::CoordsFlag & Effect::ShaderFlag);

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

  vertPath = "cornerpin.vert";
  fragPath = "cornerpin.frag";
}

void CornerPinEffect::process_coords(double timecode, GLTextureCoords &coords, int) {
  coords.vertexTopLeftX += top_left_x->GetDoubleAt(timecode);
  coords.vertexTopLeftY += top_left_y->GetDoubleAt(timecode);

  coords.vertexTopRightX += top_right_x->GetDoubleAt(timecode);
  coords.vertexTopRightY += top_right_y->GetDoubleAt(timecode);

  coords.vertexBottomLeftX += bottom_left_x->GetDoubleAt(timecode);
  coords.vertexBottomLeftY += bottom_left_y->GetDoubleAt(timecode);

  coords.vertexBottomRightX += bottom_right_x->GetDoubleAt(timecode);
  coords.vertexBottomRightY += bottom_right_y->GetDoubleAt(timecode);
}

void CornerPinEffect::process_shader(double timecode, GLTextureCoords &coords, int) {
  glslProgram->setUniformValue("p0", GLfloat(coords.vertexBottomLeftX), GLfloat(coords.vertexBottomLeftY));
  glslProgram->setUniformValue("p1", GLfloat(coords.vertexBottomRightX), GLfloat(coords.vertexBottomRightY));
  glslProgram->setUniformValue("p2", GLfloat(coords.vertexTopLeftX), GLfloat(coords.vertexTopLeftY));
  glslProgram->setUniformValue("p3", GLfloat(coords.vertexTopRightX), GLfloat(coords.vertexTopRightY));
  glslProgram->setUniformValue("perspective", perspective->GetBoolAt(timecode));
}

void CornerPinEffect::gizmo_draw(double, GLTextureCoords &coords) {
  top_left_gizmo->world_pos[0] = QPoint(coords.vertexTopLeftX, coords.vertexTopLeftY);
  top_right_gizmo->world_pos[0] = QPoint(coords.vertexTopRightX, coords.vertexTopRightY);
  bottom_right_gizmo->world_pos[0] = QPoint(coords.vertexBottomRightX, coords.vertexBottomRightY);
  bottom_left_gizmo->world_pos[0] = QPoint(coords.vertexBottomLeftX, coords.vertexBottomLeftY);
}

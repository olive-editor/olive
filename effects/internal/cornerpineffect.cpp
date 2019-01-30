#include "cornerpineffect.h"

#include "io/path.h"
#include "project/clip.h"
#include "debug.h"

CornerPinEffect::CornerPinEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
	enable_coords = true;
	enable_shader = true;

	EffectRow* top_left = add_row(tr("Top Left"));
	top_left_x = top_left->add_field(EFFECT_FIELD_DOUBLE, "topleftx");
	top_left_y = top_left->add_field(EFFECT_FIELD_DOUBLE, "toplefty");

	EffectRow* top_right = add_row(tr("Top Right"));
	top_right_x = top_right->add_field(EFFECT_FIELD_DOUBLE, "toprightx");
	top_right_y = top_right->add_field(EFFECT_FIELD_DOUBLE, "toprighty");

	EffectRow* bottom_left = add_row(tr("Bottom Left"));
	bottom_left_x = bottom_left->add_field(EFFECT_FIELD_DOUBLE, "bottomleftx");
	bottom_left_y = bottom_left->add_field(EFFECT_FIELD_DOUBLE, "bottomlefty");

	EffectRow* bottom_right = add_row(tr("Bottom Right"));
	bottom_right_x = bottom_right->add_field(EFFECT_FIELD_DOUBLE, "bottomrightx");
	bottom_right_y = bottom_right->add_field(EFFECT_FIELD_DOUBLE, "bottomrighty");

	perspective = add_row(tr("Perspective"))->add_field(EFFECT_FIELD_BOOL, "perspective");
	perspective->set_bool_value(true);

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
	coords.vertexTopLeftX += top_left_x->get_double_value(timecode);
	coords.vertexTopLeftY += top_left_y->get_double_value(timecode);

	coords.vertexTopRightX += top_right_x->get_double_value(timecode);
	coords.vertexTopRightY += top_right_y->get_double_value(timecode);

	coords.vertexBottomLeftX += bottom_left_x->get_double_value(timecode);
	coords.vertexBottomLeftY += bottom_left_y->get_double_value(timecode);

	coords.vertexBottomRightX += bottom_right_x->get_double_value(timecode);
	coords.vertexBottomRightY += bottom_right_y->get_double_value(timecode);
}

void CornerPinEffect::process_shader(double timecode, GLTextureCoords &coords, int) {
	glslProgram->setUniformValue("p0", GLfloat(coords.vertexBottomLeftX), GLfloat(coords.vertexBottomLeftY));
	glslProgram->setUniformValue("p1", GLfloat(coords.vertexBottomRightX), GLfloat(coords.vertexBottomRightY));
	glslProgram->setUniformValue("p2", GLfloat(coords.vertexTopLeftX), GLfloat(coords.vertexTopLeftY));
	glslProgram->setUniformValue("p3", GLfloat(coords.vertexTopRightX), GLfloat(coords.vertexTopRightY));
	glslProgram->setUniformValue("perspective", perspective->get_bool_value(timecode));
}

void CornerPinEffect::gizmo_draw(double, GLTextureCoords &coords) {
	top_left_gizmo->world_pos[0] = QPoint(coords.vertexTopLeftX, coords.vertexTopLeftY);
	top_right_gizmo->world_pos[0] = QPoint(coords.vertexTopRightX, coords.vertexTopRightY);
	bottom_right_gizmo->world_pos[0] = QPoint(coords.vertexBottomRightX, coords.vertexBottomRightY);
	bottom_left_gizmo->world_pos[0] = QPoint(coords.vertexBottomLeftX, coords.vertexBottomLeftY);
}

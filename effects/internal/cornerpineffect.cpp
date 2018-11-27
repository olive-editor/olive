#include "cornerpineffect.h"

#include "io/path.h"
#include "project/clip.h"
#include "debug.h"

CornerPinEffect::CornerPinEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
    enable_coords = true;
	enable_shader = true;

    EffectRow* top_left = add_row("Top Left:");
    top_left_x = top_left->add_field(EFFECT_FIELD_DOUBLE, "topleftx");
    top_left_y = top_left->add_field(EFFECT_FIELD_DOUBLE, "toplefty");

    EffectRow* top_right = add_row("Top Right:");
    top_right_x = top_right->add_field(EFFECT_FIELD_DOUBLE, "toprightx");
    top_right_y = top_right->add_field(EFFECT_FIELD_DOUBLE, "toprighty");

    EffectRow* bottom_left = add_row("Bottom Left:");
    bottom_left_x = bottom_left->add_field(EFFECT_FIELD_DOUBLE, "bottomleftx");
    bottom_left_y = bottom_left->add_field(EFFECT_FIELD_DOUBLE, "bottomlefty");

    EffectRow* bottom_right = add_row("Bottom Right:");
    bottom_right_x = bottom_right->add_field(EFFECT_FIELD_DOUBLE, "bottomrightx");
    bottom_right_y = bottom_right->add_field(EFFECT_FIELD_DOUBLE, "bottomrighty");

    perspective = add_row("Perspective:")->add_field(EFFECT_FIELD_BOOL, "perspective");
    perspective->set_bool_value(true);

    top_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
    top_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);
    bottom_left_gizmo = add_gizmo(GIZMO_TYPE_DOT);
    bottom_right_gizmo = add_gizmo(GIZMO_TYPE_DOT);

	vertPath = "cornerpin.vert";
	fragPath = "cornerpin.frag";
}

void CornerPinEffect::process_coords(double timecode, GLTextureCoords &coords, int data) {
    coords.vertexTopLeftX += top_left_x->get_double_value(timecode);
    coords.vertexTopLeftY += top_left_y->get_double_value(timecode);

    coords.vertexTopRightX += top_right_x->get_double_value(timecode);
    coords.vertexTopRightY += top_right_y->get_double_value(timecode);

    coords.vertexBottomLeftX += bottom_left_x->get_double_value(timecode);
    coords.vertexBottomLeftY += bottom_left_y->get_double_value(timecode);

    coords.vertexBottomRightX += bottom_right_x->get_double_value(timecode);
	coords.vertexBottomRightY += bottom_right_y->get_double_value(timecode);
}

void CornerPinEffect::process_shader(double timecode, GLTextureCoords &coords) {
	glslProgram->setUniformValue("p0", (GLfloat) coords.vertexBottomLeftX, (GLfloat) coords.vertexBottomLeftY);
	glslProgram->setUniformValue("p1", (GLfloat) coords.vertexBottomRightX, (GLfloat) coords.vertexBottomRightY);
	glslProgram->setUniformValue("p2", (GLfloat) coords.vertexTopLeftX, (GLfloat) coords.vertexTopLeftY);
	glslProgram->setUniformValue("p3", (GLfloat) coords.vertexTopRightX, (GLfloat) coords.vertexTopRightY);
    glslProgram->setUniformValue("perspective", perspective->get_bool_value(timecode));
}

void CornerPinEffect::gizmo_draw(double timecode, GLTextureCoords &coords) {
    top_left_gizmo->set_pos(coords.vertexTopLeftX, coords.vertexTopLeftY);
    top_right_gizmo->set_pos(coords.vertexTopRightX, coords.vertexTopRightY);
    bottom_right_gizmo->set_pos(coords.vertexBottomRightX, coords.vertexBottomRightY);
    bottom_left_gizmo->set_pos(coords.vertexBottomLeftX, coords.vertexBottomLeftY);
}

void CornerPinEffect::gizmo_move(EffectGizmo *sender, int x_movement, int y_movement, double timecode) {
    if (sender == bottom_right_gizmo) {
        bottom_right_x->set_double_value(bottom_right_x->get_double_value(timecode) + x_movement);
        bottom_right_y->set_double_value(bottom_right_y->get_double_value(timecode) + y_movement);
    } else if (sender == top_left_gizmo) {
        top_left_x->set_double_value(top_left_x->get_double_value(timecode) + x_movement);
        top_left_y->set_double_value(top_left_y->get_double_value(timecode) + y_movement);
    } else if (sender == bottom_left_gizmo) {
        bottom_left_x->set_double_value(bottom_left_x->get_double_value(timecode) + x_movement);
        bottom_left_y->set_double_value(bottom_left_y->get_double_value(timecode) + y_movement);
    } else if (sender == top_right_gizmo) {
        top_right_x->set_double_value(top_right_x->get_double_value(timecode) + x_movement);
        top_right_y->set_double_value(top_right_y->get_double_value(timecode) + y_movement);
    }
}

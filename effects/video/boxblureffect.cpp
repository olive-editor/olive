#include "boxblureffect.h"

#include "project/clip.h"

BoxBlurEffect::BoxBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_BOXBLUR_EFFECT) {
	enable_shader = true;

    radius_val = add_row("Radius:")->add_field(EFFECT_FIELD_DOUBLE);
//    iteration_val = add_row("Iterations:")->add_field(EFFECT_FIELD_DOUBLE);
    horiz_val = add_row("Horizontal:")->add_field(EFFECT_FIELD_BOOL);
    vert_val = add_row("Vertical:")->add_field(EFFECT_FIELD_BOOL);

    radius_val->set_double_default_value(9);
    radius_val->set_double_minimum_value(0);

//    iteration_val->set_double_default_value(1);
//    iteration_val->set_double_minimum_value(0);

    horiz_val->set_bool_value(true);
    vert_val->set_bool_value(true);

	vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/boxblureffect.frag";
	/*vert.compileSourceFile(":/shaders/common.vert");
    frag.compileSourceFile(":/shaders/boxblureffect.frag");
    program.addShader(&vert);
    program.addShader(&frag);
	program.link();*/

    connect(radius_val, SIGNAL(changed()), this, SLOT(field_changed()));
//    connect(iteration_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(horiz_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(vert_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void BoxBlurEffect::process_shader(double timecode) {
	glslProgram->setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
	glslProgram->setUniformValue("radius", (GLfloat) radius_val->get_double_value(timecode));
	glslProgram->setUniformValue("horiz_blur", horiz_val->get_bool_value(timecode));
	glslProgram->setUniformValue("vert_blur", vert_val->get_bool_value(timecode));
}

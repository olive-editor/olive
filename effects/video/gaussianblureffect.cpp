#include "gaussianblureffect.h"

#include "project/clip.h"

#include <QImage>

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT) {
	enable_shader = true;

    radius_val = add_row("Radius:")->add_field(EFFECT_FIELD_DOUBLE);
    sigma_val = add_row("Sigma:")->add_field(EFFECT_FIELD_DOUBLE);
	horiz_val = add_row("Horizontal:")->add_field(EFFECT_FIELD_BOOL);
	vert_val = add_row("Vertical:")->add_field(EFFECT_FIELD_BOOL);

    radius_val->set_double_minimum_value(0);
    sigma_val->set_double_minimum_value(0);

    radius_val->set_double_default_value(9);
    sigma_val->set_double_default_value(5.5);

	horiz_val->set_bool_value(true);
	vert_val->set_bool_value(true);

	vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/gaussianblureffect.frag";

    connect(radius_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(sigma_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(horiz_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(vert_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void GaussianBlurEffect::process_shader(double timecode) {
	glslProgram->setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
	glslProgram->setUniformValue("radius", (GLfloat) radius_val->get_double_value(timecode));
	glslProgram->setUniformValue("sigma", (GLfloat) sigma_val->get_double_value(timecode));
	glslProgram->setUniformValue("horiz_blur", horiz_val->get_bool_value(timecode));
	glslProgram->setUniformValue("vert_blur", vert_val->get_bool_value(timecode));
}

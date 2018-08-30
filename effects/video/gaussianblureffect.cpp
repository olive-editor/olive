#include "gaussianblureffect.h"

#include "project/clip.h"

#include <QImage>

GaussianBlurEffect::GaussianBlurEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_GAUSSIANBLUR_EFFECT), frag(QOpenGLShader::Fragment), vert(QOpenGLShader::Vertex), bound(false) {
	enable_opengl = true;
	enable_image = false;

    kernelsz_val = add_row("Kernel Size:")->add_field(EFFECT_FIELD_DOUBLE);
    sigma_val = add_row("Sigma:")->add_field(EFFECT_FIELD_DOUBLE);
	horiz_val = add_row("Horizontal:")->add_field(EFFECT_FIELD_BOOL);
	vert_val = add_row("Vertical:")->add_field(EFFECT_FIELD_BOOL);

    kernelsz_val->set_double_minimum_value(0);
    sigma_val->set_double_minimum_value(0);

    kernelsz_val->set_double_default_value(9);
    sigma_val->set_double_default_value(5.5);

	horiz_val->set_bool_value(true);
	vert_val->set_bool_value(true);

    vert.compileSourceFile(":/shaders/common.vert");
    frag.compileSourceFile(":/shaders/gaussianblureffect.frag");
	program.addShader(&vert);
	program.addShader(&frag);
    program.link();

    connect(kernelsz_val, SIGNAL(changed()), this, SLOT(field_changed()));
    connect(sigma_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(horiz_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(vert_val, SIGNAL(changed()), this, SLOT(field_changed()));

	refresh();
}

void GaussianBlurEffect::process_gl(double timecode, GLTextureCoords &coords) {
    program.bind();

    program.setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
    program.setUniformValue("kernel_size", (GLfloat) kernelsz_val->get_double_value(timecode));
    program.setUniformValue("sigma", (GLfloat) sigma_val->get_double_value(timecode));
    program.setUniformValue("horiz_blur", horiz_val->get_bool_value(timecode));
    program.setUniformValue("vert_blur", vert_val->get_bool_value(timecode));
}

void GaussianBlurEffect::clean_gl() {
    program.release();
}

#include "chromakeyeffect.h"

ChromaKeyEffect::ChromaKeyEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_CHROMAKEY_EFFECT), vert(QOpenGLShader::Vertex), frag(QOpenGLShader::Fragment) {
	enable_opengl = true;

	color_field = add_row("Color:")->add_field(EFFECT_FIELD_COLOR);
	tolerance_field = add_row("Tolerance:")->add_field(EFFECT_FIELD_DOUBLE);

	color_field->set_color_value(Qt::green);
	tolerance_field->set_double_default_value(10);

	tolerance_field->set_double_minimum_value(0);
	tolerance_field->set_double_maximum_value(100);

    vert.compileSourceFile(":/shaders/common.vert");
    frag.compileSourceFile(":/shaders/chromakeyeffect.frag");
	program.addShader(&vert);
	program.addShader(&frag);
	program.link();

	connect(color_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(tolerance_field, SIGNAL(changed()), this, SLOT(field_changed()));
}

void ChromaKeyEffect::process_gl(double timecode, GLTextureCoords&) {
	program.bind();
	program.setUniformValue("keyColor", color_field->get_color_value(timecode));
	program.setUniformValue("threshold", (GLfloat) (tolerance_field->get_double_value(timecode)*0.01));
}

void ChromaKeyEffect::clean_gl() {
	program.release();
}

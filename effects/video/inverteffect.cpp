#include "inverteffect.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QOpenGLFunctions>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT), vert(QOpenGLShader::Vertex), frag(QOpenGLShader::Fragment) {
	enable_opengl = true;

	EffectRow* amount_row = add_row("Amount:");
	amount_val = amount_row->add_field(EFFECT_FIELD_DOUBLE);
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);

	// set defaults	
	amount_val->set_double_default_value(100);

	connect(amount_val, SIGNAL(changed()), this, SLOT(field_changed()));

    vert.compileSourceFile(":/shaders/common.vert");
    frag.compileSourceFile(":/shaders/inverteffect.frag");
	program.addShader(&vert);
	program.addShader(&frag);
	program.link();
}

void InvertEffect::process_gl(double timecode, GLTextureCoords&) {
	program.bind();
	program.setUniformValue("amount_val", (GLfloat) (amount_val->get_double_value(timecode)*0.01));
}

void InvertEffect::clean_gl() {
	program.release();
}

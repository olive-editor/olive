#include "inverteffect.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QOpenGLFunctions>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT) {
	enable_shader = true;

	EffectRow* amount_row = add_row("Amount:");
	amount_val = amount_row->add_field(EFFECT_FIELD_DOUBLE);
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);

	// set defaults	
	amount_val->set_double_default_value(100);

	connect(amount_val, SIGNAL(changed()), this, SLOT(field_changed()));

	vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/inverteffect.frag";
}

void InvertEffect::process_shader(double timecode) {
	glslProgram->setUniformValue("amount_val", (GLfloat) (amount_val->get_double_value(timecode)*0.01));
}

#include "effects/effects.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT) {

	EffectRow* amount_row = add_row("Amount:");
	amount_val = amount_row->add_field(EFFECT_FIELD_DOUBLE);
	amount_val->set_double_minimum_value(0);
	amount_val->set_double_maximum_value(100);

	// set defaults	
	amount_val->set_double_default_value(100);
}

Effect* InvertEffect::copy(Clip* c) {
	/*InvertEffect* i = new InvertEffect(c);
	i->amount_val->set_value(amount_val->value());
	return i;*/
	return NULL;
}

void InvertEffect::process_gl(int* anchor_x, int* anchor_y) {

//	gl_FragColor = vec4(1.0 - textureColor.r, 1.0 -textureColor.g, 1.0 -textureColor.b, 1);
}

#include "effects/effects.h"

#include "ui/labelslider.h"

#include <QLabel>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

InvertEffect::InvertEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_INVERT_EFFECT) {
	ui_layout->addWidget(new QLabel("Amount:"), 0, 0);
	amount_val = new LabelSlider();
	ui_layout->addWidget(amount_val, 0, 1);

	// set defaults
	amount_val->set_minimum_value(0);
	amount_val->set_maximum_value(100);
	init();

	connect(amount_val, SIGNAL(valueChanged()), this, SLOT(field_changed()));
}

void InvertEffect::init() {
	amount_val->set_default_value(100);
}

Effect* InvertEffect::copy(Clip* c) {
	InvertEffect* i = new InvertEffect(c);
	i->amount_val->set_value(amount_val->value());
	return i;
}

void InvertEffect::load(QXmlStreamReader *stream) {
	const QXmlStreamAttributes& attr = stream->attributes();
	for (int i=0;i<attr.size();i++) {
		const QXmlStreamAttribute& a = attr.at(i);
		if (a.name() == "amount") {
			amount_val->set_value(a.value().toDouble());
		}
	}
}

void InvertEffect::save(QXmlStreamWriter *stream) {
	stream->writeAttribute("amount", QString::number(amount_val->value()));
}

void InvertEffect::process_gl(int* anchor_x, int* anchor_y) {

//	gl_FragColor = vec4(1.0 - textureColor.r, 1.0 -textureColor.g, 1.0 -textureColor.b, 1);
}

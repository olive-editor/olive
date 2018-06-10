#include "effects/effects.h"

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>

#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c) : Effect(c) {
    setup_effect(EFFECT_TYPE_AUDIO, AUDIO_PAN_EFFECT);

    QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Pan:"), 0, 0);
	pan_val = new QSpinBox();
	pan_val->setMinimum(-100);
	pan_val->setMaximum(100);
	ui_layout->addWidget(pan_val, 0, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
	pan_val->setValue(0);
}

Effect* PanEffect::copy() {
    PanEffect* p = new PanEffect(parent_clip);
    p->pan_val->setValue(pan_val->value());
    return p;
}

void PanEffect::save(QXmlStreamWriter *stream) {
    stream->writeTextElement("pan", QString::number(pan_val->value()));
}

void PanEffect::process_audio(uint8_t *samples, int nb_bytes) {
	for (int i=0;i<nb_bytes;i+=4) {
		int16_t left_sample = (int16_t) ((samples[i+1] << 8) | samples[i]);
		int16_t right_sample = (int16_t) ((samples[i+3] << 8) | samples[i+2]);

		float val = pan_val->value()*0.01;
		if (val < 0) {
			// affect right channel
			right_sample *= (1-abs(val));
		} else {
			// affect left channel
			left_sample *= (1-val);
		}

		samples[i+3] = (uint8_t) (right_sample >> 8);
		samples[i+2] = (uint8_t) right_sample;
		samples[i+1] = (uint8_t) (left_sample >> 8);
		samples[i] = (uint8_t) left_sample;
	}
}

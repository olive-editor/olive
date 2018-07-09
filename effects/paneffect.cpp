#include "effects/effects.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <cmath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c) : Effect(c) {
    setup_effect(EFFECT_TYPE_AUDIO, AUDIO_PAN_EFFECT);

    QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Pan:"), 0, 0);
    pan_val = new LabelSlider();
    pan_val->set_minimum_value(-100);
    pan_val->set_maximum_value(100);
	ui_layout->addWidget(pan_val, 0, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
    pan_val->set_value(0);
}

Effect* PanEffect::copy(Clip* c) {
    PanEffect* p = new PanEffect(c);
    p->pan_val->set_value(pan_val->value());
    return p;
}

void PanEffect::load(QXmlStreamReader *stream) {
    while (!(stream->isEndElement() && stream->name() == "effect") && !stream->atEnd()) {
        stream->readNext();
        if (stream->isStartElement() && stream->name() == "pan") {
            stream->readNext();
            pan_val->set_value(stream->text().toFloat());
        }
    }
}

void PanEffect::save(QXmlStreamWriter *stream) {
    stream->writeTextElement("pan", QString::number(pan_val->value()));
}

void PanEffect::process_audio(quint8 *samples, int nb_bytes) {
    if (pan_val->value() != 0) {
        for (int i=0;i<nb_bytes;i+=4) {
            qint16 left_sample = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
            qint16 right_sample = (qint16) (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));

            float val = qPow(pan_val->value()*0.01f, 3);
            if (val < 0) {
                // affect right channel
                right_sample *= (1-std::abs(val));
            } else {
                // affect left channel
                left_sample *= (1-val);
            }

            samples[i+3] = (quint8) (right_sample >> 8);
            samples[i+2] = (quint8) right_sample;
            samples[i+1] = (quint8) (left_sample >> 8);
            samples[i] = (quint8) left_sample;
        }
    }
}

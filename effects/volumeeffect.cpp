#include "effects/effects.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <QDebug>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

VolumeEffect::VolumeEffect(Clip* c) : Effect(c) {
    setup_effect(EFFECT_TYPE_AUDIO, AUDIO_VOLUME_EFFECT);

	QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Volume:"), 0, 0);
    volume_val = new LabelSlider();
    volume_val->set_minimum_value(0);
    volume_val->set_maximum_value(400);
	ui_layout->addWidget(volume_val, 0, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
    volume_val->set_value(100);
}

Effect* VolumeEffect::copy(Clip* c) {
    VolumeEffect* v = new VolumeEffect(c);
    v->volume_val->set_value(volume_val->value());
    return v;
}

void VolumeEffect::load(QXmlStreamReader* stream) {
    while (!(stream->isEndElement() && stream->name() == "effect") && !stream->atEnd()) {
        stream->readNext();
        if (stream->isStartElement() && stream->name() == "volume") {
            stream->readNext();
            volume_val->set_value(stream->text().toFloat());
        }
    }
}

void VolumeEffect::save(QXmlStreamWriter* stream) {
    stream->writeTextElement("volume", QString::number(volume_val->value()));
}

void VolumeEffect::process_audio(quint8* samples, int nb_bytes) {
    if (volume_val->value() != 100) {
        for (int i=0;i<nb_bytes;i+=2) {
            qint16 samp = (qint16) ((samples[i+1] << 8) | samples[i]);
            float val = qPow(volume_val->value()*0.01f, 3);
            samp *= val;
            samples[i+1] = (quint8) (samp >> 8);
            samples[i] = (quint8) samp;
        }
    }
}

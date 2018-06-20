#include "effects/effects.h"

#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>

#include "ui/collapsiblewidget.h"

VolumeEffect::VolumeEffect(Clip* c) : Effect(c) {
    setup_effect(EFFECT_TYPE_AUDIO, AUDIO_VOLUME_EFFECT);

	QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Volume:"), 0, 0);
	volume_val = new QSpinBox();
	volume_val->setMinimum(0);
    volume_val->setMaximum(400);
	ui_layout->addWidget(volume_val, 0, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
	volume_val->setValue(100);
}

Effect* VolumeEffect::copy(Clip* c) {
    VolumeEffect* v = new VolumeEffect(c);
    v->volume_val->setValue(volume_val->value());
    return v;
}

void VolumeEffect::load(QXmlStreamReader* stream) {
    while (!(stream->isEndElement() && stream->name() == "effect") && !stream->atEnd()) {
        stream->readNext();
        if (stream->isStartElement() && stream->name() == "volume") {
            stream->readNext();
            volume_val->setValue(stream->text().toInt());
        }
    }
}

void VolumeEffect::save(QXmlStreamWriter* stream) {
    stream->writeTextElement("volume", QString::number(volume_val->value()));
}

void VolumeEffect::process_audio(quint8* samples, int nb_bytes) {
    if (volume_val->value() != 0) {
        for (int i=0;i<nb_bytes;i+=2) {
            qint16 samp = (qint16) ((samples[i+1] << 8) | samples[i]);
            samp *= (volume_val->value()*0.01f);
            samples[i+1] = (quint8) (samp >> 8);
            samples[i] = (quint8) samp;
        }
    }
}

#include "effects/effects.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <QOpenGLFunctions>
#include <QDebug>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/timeline.h"

ShakeEffect::ShakeEffect(Clip *c) : Effect(c) {
    setup_effect(EFFECT_TYPE_VIDEO, VIDEO_SHAKE_EFFECT);

    QGridLayout* ui_layout = new QGridLayout();

    ui_layout->addWidget(new QLabel("Intensity:"), 0, 0);
    intensity_val = new LabelSlider();
    intensity_val->set_minimum_value(0);
    ui_layout->addWidget(intensity_val, 0, 1);

    ui_layout->addWidget(new QLabel("Rotation:"), 1, 0);
    rotation_val = new LabelSlider();
    rotation_val->set_minimum_value(0);
    ui_layout->addWidget(rotation_val, 1, 1);

    ui_layout->addWidget(new QLabel("Frequency:"), 2, 0);
    frequency_val = new LabelSlider();
    frequency_val->set_minimum_value(0);
    ui_layout->addWidget(frequency_val, 2, 1);

    ui->setLayout(ui_layout);

    container->setContents(ui);

    // set defaults
    intensity_val->set_value(50);
    rotation_val->set_value(0);
    frequency_val->set_value(10);

    set_values();

    connect(intensity_val, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(rotation_val, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(frequency_val, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(intensity_val, SIGNAL(valueChanged()), this, SLOT(set_values()));
    connect(rotation_val, SIGNAL(valueChanged()), this, SLOT(set_values()));
    connect(frequency_val, SIGNAL(valueChanged()), this, SLOT(set_values()));
}

void ShakeEffect::set_values() {
    shake_limit = qRound(parent_clip->sequence->frame_rate / frequency_val->value());
    shake_progress = shake_limit;
    next_x = 0;
    next_y = 0;
    next_rot = 0;
}

Effect* ShakeEffect::copy(Clip* c) {
    ShakeEffect* e = new ShakeEffect(c);
    e->intensity_val->set_value(intensity_val->value());
    e->rotation_val->set_value(rotation_val->value());
    e->frequency_val->set_value(frequency_val->value());
    return e;
}

void ShakeEffect::load(QXmlStreamReader* reader) {
    const QXmlStreamAttributes& attr = reader->attributes();
    for (int i=0;i<attr.size();i++) {
        const QXmlStreamAttribute& a = attr.at(i);
        if (a.name() == "intensity") {
            intensity_val->set_value(a.value().toFloat());
        } else if (a.name() == "rotation") {
            rotation_val->set_value(a.value().toFloat());
        } else if (a.name() == "frequency") {
            frequency_val->set_value(a.value().toFloat());
        }
    }
}

void ShakeEffect::save(QXmlStreamWriter* stream) {
    stream->writeAttribute("intensity", QString::number(intensity_val->value()));
    stream->writeAttribute("rotation", QString::number(rotation_val->value()));
    stream->writeAttribute("frequency", QString::number(frequency_val->value()));
}

void ShakeEffect::process_gl(int*, int*) {
    if (shake_progress > shake_limit) {
        if (intensity_val->value() > 0) {
            prev_x = next_x;
            prev_y = next_y;
            next_x = (qrand() % (int) (intensity_val->value() * 2)) - intensity_val->value();
            next_y = (qrand() % (int) (intensity_val->value() * 2)) - intensity_val->value();
        } else {
            prev_x = 0;
            prev_y = 0;
            next_x = 0;
            next_y = 0;
            offset_x = 0;
            offset_y = 0;
        }
        if (rotation_val->value() > 0) {
            prev_rot = next_rot;
            next_rot = (qrand() % (int) (rotation_val->value() * 2)) - rotation_val->value();
        } else {
            prev_rot = 0;
            next_rot = 0;
            offset_rot = 0;
        }
        shake_progress = 0;
    }
    t = 1 - qPow(((double) shake_progress / (double) shake_limit)-1, 2);
    offset_x = lerp(prev_x, next_x, t);
    offset_y = lerp(prev_y, next_y, t);
    offset_rot = lerp(prev_rot, next_rot, t);
    glTranslatef(offset_x, offset_y, 0);
    glRotatef(offset_rot, 0, 0, 1);
    shake_progress++;
}

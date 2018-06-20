#include "effects/effects.h"

#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QOpenGLFunctions>

#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "ui/labelslider.h"

TransformEffect::TransformEffect(Clip* c) : Effect(c) {
    setup_effect(EFFECT_TYPE_VIDEO, VIDEO_TRANSFORM_EFFECT);

	QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Position:"), 0, 0);
    position_x = new LabelSlider();
    ui_layout->addWidget(position_x, 0, 1);

    position_y = new LabelSlider();
	ui_layout->addWidget(position_y, 0, 2);

	ui_layout->addWidget(new QLabel("Scale:"), 1, 0);
    scale_x = new LabelSlider();
    scale_x->set_minimum_value(0);
    scale_x->set_maximum_value(1200);
	ui_layout->addWidget(scale_x, 1, 1);
    scale_y = new LabelSlider();
    scale_y->set_minimum_value(0);
    scale_y->set_maximum_value(1200);
	ui_layout->addWidget(scale_y, 1, 2);

	uniform_scale_box = new QCheckBox();
	uniform_scale_box->setText("Uniform Scale");
	ui_layout->addWidget(uniform_scale_box, 2, 1);

	ui_layout->addWidget(new QLabel("Rotation:"), 3, 0);
    rotation = new LabelSlider();
	ui_layout->addWidget(rotation, 3, 1);

	ui_layout->addWidget(new QLabel("Anchor Point:"), 4, 0);
    anchor_x_box = new LabelSlider();
	ui_layout->addWidget(anchor_x_box, 4, 1);
    anchor_y_box = new LabelSlider();
	ui_layout->addWidget(anchor_y_box, 4, 2);

	ui_layout->addWidget(new QLabel("Opacity:"), 5, 0);
    opacity = new LabelSlider();
    opacity->set_minimum_value(0);
    opacity->set_maximum_value(100);
	ui_layout->addWidget(opacity, 5, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
    position_x->set_default_value(c->sequence->width/2);
    position_y->set_default_value(c->sequence->height/2);
    scale_x->set_default_value(100);
    scale_y->set_default_value(100);
	scale_y->setEnabled(false);
	uniform_scale_box->setChecked(true);
    default_anchor_x = c->media_stream->video_width/2;
    default_anchor_y = c->media_stream->video_height/2;
    anchor_x_box->set_default_value(default_anchor_x);
    anchor_y_box->set_default_value(default_anchor_y);
    opacity->set_default_value(100);

    connect(position_x, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(position_y, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(rotation, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(scale_x, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(scale_y, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(anchor_x_box, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(anchor_y_box, SIGNAL(valueChanged()), this, SLOT(field_changed()));
    connect(opacity, SIGNAL(valueChanged()), this, SLOT(field_changed()));
	connect(uniform_scale_box, SIGNAL(toggled(bool)), this, SLOT(toggle_uniform_scale(bool)));
	connect(uniform_scale_box, SIGNAL(toggled(bool)), this, SLOT(field_changed()));
}

Effect* TransformEffect::copy(Clip* c) {
    TransformEffect* t = new TransformEffect(c);
    t->position_x->set_value(position_x->value());
    t->position_y->set_value(position_y->value());
    t->scale_x->set_value(scale_x->value());
    t->scale_y->set_value(scale_y->value());
    t->uniform_scale_box->setChecked(uniform_scale_box->isChecked());
    t->rotation->set_value(rotation->value());
    t->anchor_x_box->set_value(anchor_x_box->value());
    t->anchor_y_box->set_value(anchor_y_box->value());
    t->opacity->set_value(opacity->value());
    return t;
}

void TransformEffect::toggle_uniform_scale(bool enabled) {
	scale_y->setEnabled(!enabled);
}

void TransformEffect::process_gl(int* anchor_x, int* anchor_y) {
	// position
	glTranslatef(position_x->value()-(parent_clip->sequence->width/2), position_y->value()-(parent_clip->sequence->height/2), 0);

	// anchor point
    *anchor_x += (anchor_x_box->value()-default_anchor_x);
    *anchor_y += (anchor_y_box->value()-default_anchor_y);

	// rotation
	glRotatef(rotation->value(), 0, 0, 1);

	// scale
	float sx = scale_x->value()*0.01;
	float sy = (uniform_scale_box->isChecked()) ? sx : scale_y->value()*0.01;
	glScalef(sx, sy, 1);

	// opacity
	glColor4f(1.0, 1.0, 1.0, opacity->value()*0.01);
}

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

TransformEffect::TransformEffect(Clip* c) : Effect(c) {
	container = new CollapsibleWidget();
	container->setText(video_effect_names[VIDEO_TRANSFORM_EFFECT]);

	ui = new QWidget();

	QGridLayout* ui_layout = new QGridLayout();

	ui_layout->addWidget(new QLabel("Position:"), 0, 0);
	position_x = new QSpinBox();
	position_x->setMinimum(-QWIDGETSIZE_MAX);
	position_x->setMaximum(QWIDGETSIZE_MAX);
	ui_layout->addWidget(position_x, 0, 1);
	position_y = new QSpinBox();
	position_y->setMinimum(-QWIDGETSIZE_MAX);
	position_y->setMaximum(QWIDGETSIZE_MAX);
	ui_layout->addWidget(position_y, 0, 2);

	ui_layout->addWidget(new QLabel("Scale:"), 1, 0);
	scale_x = new QSpinBox();
	scale_x->setMinimum(0);
	scale_x->setMaximum(1200);
	ui_layout->addWidget(scale_x, 1, 1);
	scale_y = new QSpinBox();
	scale_y->setMinimum(0);
	scale_y->setMaximum(1200);
	ui_layout->addWidget(scale_y, 1, 2);

	uniform_scale_box = new QCheckBox();
	uniform_scale_box->setText("Uniform Scale");
	ui_layout->addWidget(uniform_scale_box, 2, 1);

	ui_layout->addWidget(new QLabel("Rotation:"), 3, 0);
	rotation = new QSpinBox();
	rotation->setMinimum(-QWIDGETSIZE_MAX);
	rotation->setMaximum(QWIDGETSIZE_MAX);
	ui_layout->addWidget(rotation, 3, 1);

	ui_layout->addWidget(new QLabel("Anchor Point:"), 4, 0);
	anchor_x_box = new QSpinBox();
	anchor_x_box->setMinimum(-QWIDGETSIZE_MAX);
	anchor_x_box->setMaximum(QWIDGETSIZE_MAX);
	ui_layout->addWidget(anchor_x_box, 4, 1);
	anchor_y_box = new QSpinBox();
	anchor_y_box->setMinimum(-QWIDGETSIZE_MAX);
	anchor_y_box->setMaximum(QWIDGETSIZE_MAX);
	ui_layout->addWidget(anchor_y_box, 4, 2);

	ui_layout->addWidget(new QLabel("Opacity:"), 5, 0);
	opacity = new QSpinBox();
	opacity->setMinimum(0);
	opacity->setMaximum(100);
	ui_layout->addWidget(opacity, 5, 1);

	ui->setLayout(ui_layout);

	container->setContents(ui);

	// set defaults
    position_x->setValue(c->sequence->width/2);
    position_y->setValue(c->sequence->height/2);
	scale_x->setValue(100);
	scale_y->setValue(100);
	scale_y->setEnabled(false);
	uniform_scale_box->setChecked(true);
	anchor_x_box->setValue(c->media_stream->video_width/2);
	anchor_y_box->setValue(c->media_stream->video_height/2);
	opacity->setValue(100);

	connect(position_x, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(position_y, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(rotation, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(scale_x, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(scale_y, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(anchor_x_box, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(anchor_y_box, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(opacity, SIGNAL(valueChanged(int)), this, SLOT(field_changed()));
	connect(uniform_scale_box, SIGNAL(toggled(bool)), this, SLOT(toggle_uniform_scale(bool)));
	connect(uniform_scale_box, SIGNAL(toggled(bool)), this, SLOT(field_changed()));
}

Effect* TransformEffect::copy() {
    TransformEffect* t = new TransformEffect(parent_clip);
    t->position_x->setValue(position_x->value());
    t->position_y->setValue(position_y->value());
    t->scale_x->setValue(scale_x->value());
    t->scale_y->setValue(scale_y->value());
    t->uniform_scale_box->setChecked(uniform_scale_box->isChecked());
    t->rotation->setValue(rotation->value());
    t->anchor_x_box->setValue(anchor_x_box->value());
    t->anchor_y_box->setValue(anchor_y_box->value());
    t->opacity->setValue(opacity->value());
    return t;
}

void TransformEffect::toggle_uniform_scale(bool enabled) {
	scale_y->setEnabled(!enabled);
}

void TransformEffect::process_gl(int* anchor_x, int* anchor_y) {
	// position
	glTranslatef(position_x->value()-(parent_clip->sequence->width/2), position_y->value()-(parent_clip->sequence->height/2), 0);

	// anchor point
	*anchor_x += anchor_x_box->value();
	*anchor_y += anchor_y_box->value();

	// rotation
	glRotatef(rotation->value(), 0, 0, 1);

	// scale
	float sx = scale_x->value()*0.01;
	float sy = (uniform_scale_box->isChecked()) ? sx : scale_y->value()*0.01;
	glScalef(sx, sy, 1);

	// opacity
	glColor4f(1.0, 1.0, 1.0, opacity->value()*0.01);
}

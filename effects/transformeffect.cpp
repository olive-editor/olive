#include "effects/effects.h"

#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QOpenGLFunctions>
#include <QComboBox>

#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "ui/labelslider.h"
#include "ui/comboboxex.h"
#include "panels/project.h"

#define BLEND_MODE_NORMAL 0
#define BLEND_MODE_SCREEN 1
#define BLEND_MODE_MULTIPLY 2
#define BLEND_MODE_OVERLAY 3

TransformEffect::TransformEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_TRANSFORM_EFFECT) {
	ui_layout->addWidget(new QLabel("Position:"), 0, 0);
    position_x = new LabelSlider();
    ui_layout->addWidget(position_x, 0, 1);

    position_y = new LabelSlider();
	ui_layout->addWidget(position_y, 0, 2);

	ui_layout->addWidget(new QLabel("Scale:"), 1, 0);
    scale_x = new LabelSlider();
    scale_x->set_minimum_value(0);
    scale_x->set_maximum_value(3000);
	ui_layout->addWidget(scale_x, 1, 1);
    scale_y = new LabelSlider();
    scale_y->set_minimum_value(0);
    scale_y->set_maximum_value(3000);
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

    ui_layout->addWidget(new QLabel("Blend Mode:"), 6, 0);
    blend_mode_box = new ComboBoxEx();
    blend_mode_box->addItem("Normal", BLEND_MODE_NORMAL);
    blend_mode_box->addItem("Overlay", BLEND_MODE_OVERLAY);
    blend_mode_box->addItem("Screen", BLEND_MODE_SCREEN);
    blend_mode_box->addItem("Multiply", BLEND_MODE_MULTIPLY);
    ui_layout->addWidget(blend_mode_box, 6, 1);

	// set defaults
    scale_y->setEnabled(false);
    uniform_scale_box->setChecked(true);
    blend_mode_box->setCurrentIndex(0);
    init();

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
    connect(uniform_scale_box, SIGNAL(clicked(bool)), this, SLOT(checkbox_command()));
    connect(blend_mode_box, SIGNAL(currentIndexChanged(int)), this, SLOT(field_changed()));
}

void TransformEffect::init() {
    if (parent_clip->sequence != NULL) {
        double default_pos_x = parent_clip->sequence->width/2;
        double default_pos_y = parent_clip->sequence->height/2;

        position_x->set_default_value(default_pos_x);
        position_y->set_default_value(default_pos_y);
        scale_x->set_default_value(100);
        scale_y->set_default_value(100);

        default_anchor_x = default_pos_x;
        default_anchor_y = default_pos_y;
        if (parent_clip->media != NULL) {
            switch (parent_clip->media_type) {
            case MEDIA_TYPE_FOOTAGE:
            {
                MediaStream* ms = static_cast<Media*>(parent_clip->media)->get_stream_from_file_index(parent_clip->media_stream);
                if (ms != NULL) {
                    default_anchor_x = ms->video_width/2;
                    default_anchor_y = ms->video_height/2;
                }
                break;
            }
            case MEDIA_TYPE_SEQUENCE:
            {
                Sequence* s = static_cast<Sequence*>(parent_clip->media);
                default_anchor_x = s->width/2;
                default_anchor_y = s->height/2;
                break;
            }
            }
        }

        anchor_x_box->set_default_value(default_anchor_x);
        anchor_y_box->set_default_value(default_anchor_y);
        opacity->set_default_value(100);
    }
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
    t->blend_mode_box->setCurrentIndex(blend_mode_box->currentIndex());
    return t;
}

void TransformEffect::load(QXmlStreamReader *stream) {
    while (!(stream->isEndElement() && stream->name() == "effect") && !stream->atEnd()) {
        stream->readNext();
        if (stream->isStartElement() && stream->name() == "posx") {
            stream->readNext();
            position_x->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "posy") {
            stream->readNext();
            position_y->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "scalex") {
            stream->readNext();
            scale_x->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "scaley") {
            stream->readNext();
            scale_y->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "uniformscale") {
            stream->readNext();
            uniform_scale_box->setChecked(stream->text() == "1");
        } else if (stream->isStartElement() && stream->name() == "rotation") {
            stream->readNext();
            rotation->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "anchorx") {
            stream->readNext();
            anchor_x_box->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "anchory") {
            stream->readNext();
            anchor_y_box->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "opacity") {
            stream->readNext();
            opacity->set_value(stream->text().toDouble());
        } else if (stream->isStartElement() && stream->name() == "blendmode") {
            stream->readNext();
            blend_mode_box->setCurrentIndex(stream->text().toInt());
        }
    }
}

void TransformEffect::save(QXmlStreamWriter *stream) {
    stream->writeTextElement("posx", QString::number(position_x->value()));
    stream->writeTextElement("posy", QString::number(position_y->value()));
    stream->writeTextElement("scalex", QString::number(scale_x->value()));
    stream->writeTextElement("scaley", QString::number(scale_y->value()));
    stream->writeTextElement("uniformscale", QString::number(uniform_scale_box->isChecked()));
    stream->writeTextElement("rotation", QString::number(rotation->value()));
    stream->writeTextElement("anchorx", QString::number(anchor_x_box->value()));
    stream->writeTextElement("anchory", QString::number(anchor_y_box->value()));
    stream->writeTextElement("opacity", QString::number(opacity->value()));
    stream->writeTextElement("blendmode", QString::number(blend_mode_box->currentIndex()));
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

    // blend mode
    switch (blend_mode_box->currentData().toInt()) {
    case BLEND_MODE_NORMAL:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case BLEND_MODE_OVERLAY:
        glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
        break;
    case BLEND_MODE_SCREEN:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        break;
    case BLEND_MODE_MULTIPLY:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    default:
        qDebug() << "[ERROR] Invalid blend mode. This is a bug - please contact developers";
    }

	// opacity
    float color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
    glColor4f(1.0, 1.0, 1.0, color[3]*(opacity->value()*0.01));
}

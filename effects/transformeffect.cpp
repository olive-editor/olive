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
	EffectRow* position_row = add_row("Position:");
	position_x = position_row->add_field(EFFECT_FIELD_DOUBLE); // position X
	position_y = position_row->add_field(EFFECT_FIELD_DOUBLE); // position Y

	EffectRow* scale_row = add_row("Scale:");
	scale_x = scale_row->add_field(EFFECT_FIELD_DOUBLE); // scale X (and Y is uniform scale is selected)
	scale_x->set_double_minimum_value(0);
	scale_x->set_double_maximum_value(3000);
	scale_y = scale_row->add_field(EFFECT_FIELD_DOUBLE); // scale Y (disabled if uniform scale is selected)
	scale_y->set_double_minimum_value(0);
	scale_y->set_double_maximum_value(3000);

	EffectRow* uniform_scale_row = add_row("Uniform Scale:");
	uniform_scale_field = uniform_scale_row->add_field(EFFECT_FIELD_BOOL); // uniform scale option

	EffectRow* rotation_row = add_row("Rotation:");
	rotation = rotation_row->add_field(EFFECT_FIELD_DOUBLE);

	EffectRow* anchor_point_row = add_row("Anchor Point:");
	anchor_x_box = anchor_point_row->add_field(EFFECT_FIELD_DOUBLE); // anchor point X
	anchor_y_box = anchor_point_row->add_field(EFFECT_FIELD_DOUBLE); // anchor point Y

	EffectRow* opacity_row = add_row("Opacity:");
	opacity = opacity_row->add_field(EFFECT_FIELD_DOUBLE); // opacity
	opacity->set_double_minimum_value(0);
	opacity->set_double_maximum_value(100);

	EffectRow* blend_mode_row = add_row("Blend Mode:");
	blend_mode_box = blend_mode_row->add_field(EFFECT_FIELD_COMBO); // blend mode
	blend_mode_box->add_combo_item("Normal", BLEND_MODE_NORMAL);
	blend_mode_box->add_combo_item("Overlay", BLEND_MODE_OVERLAY);
	blend_mode_box->add_combo_item("Screen", BLEND_MODE_SCREEN);
	blend_mode_box->add_combo_item("Multiply", BLEND_MODE_MULTIPLY);

	// set defaults
	QCheckBox* uniform_scale_box = static_cast<QCheckBox*>(uniform_scale_field->get_ui_element());
	scale_y->set_enabled(false);
	uniform_scale_field->set_bool_value(true);
	blend_mode_box->set_combo_index(0);
	refresh();

	connect(position_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(position_y, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(scale_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(scale_y, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(uniform_scale_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(rotation, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(anchor_x_box, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(anchor_y_box, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(opacity, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(blend_mode_box, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(uniform_scale_box, SIGNAL(toggled(bool)), this, SLOT(toggle_uniform_scale(bool)));
}

Effect* TransformEffect::copy(Clip* c) {
	TransformEffect* t = new TransformEffect(c);
	t->position_x->set_double_value(position_x->get_double_value());
	t->position_y->set_double_value(position_y->get_double_value());
	t->scale_x->set_double_value(scale_x->get_double_value());
	t->scale_y->set_double_value(scale_y->get_double_value());
	t->uniform_scale_field->set_bool_value(uniform_scale_field->get_bool_value());
	t->rotation->set_double_value(rotation->get_double_value());
	t->anchor_x_box->set_double_value(anchor_x_box->get_double_value());
	t->anchor_y_box->set_double_value(anchor_y_box->get_double_value());
	t->opacity->set_double_value(opacity->get_double_value());
	t->blend_mode_box->set_combo_index(blend_mode_box->get_combo_index());
	return t;
}

void TransformEffect::refresh() {
	if (parent_clip->sequence != NULL) {
        double default_pos_x = parent_clip->sequence->width/2;
        double default_pos_y = parent_clip->sequence->height/2;

		position_x->set_double_default_value(default_pos_x);
		position_y->set_double_default_value(default_pos_y);
		scale_x->set_double_default_value(100);
		scale_y->set_double_default_value(100);

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

		anchor_x_box->set_double_default_value(default_anchor_x);
		anchor_y_box->set_double_default_value(default_anchor_y);
		opacity->set_double_default_value(100);
	}
}

void TransformEffect::toggle_uniform_scale(bool enabled) {
	scale_y->set_enabled(!enabled);
}

void TransformEffect::process_gl(int* anchor_x, int* anchor_y) {
	// position
	glTranslatef(position_x->get_double_value()-(parent_clip->sequence->width/2), position_y->get_double_value()-(parent_clip->sequence->height/2), 0);

	// anchor point
	*anchor_x += (anchor_x_box->get_double_value()-default_anchor_x);
	*anchor_y += (anchor_y_box->get_double_value()-default_anchor_y);

	// rotation
	glRotatef(rotation->get_double_value(), 0, 0, 1);

	// scale
	float sx = scale_x->get_double_value()*0.01;
	float sy = (uniform_scale_field->get_bool_value()) ? sx : scale_y->get_double_value()*0.01;
    glScalef(sx, sy, 1);

    // blend mode
	switch (blend_mode_box->get_combo_data().toInt()) {
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
	glColor4f(1.0, 1.0, 1.0, color[3]*(opacity->get_double_value()*0.01));
}

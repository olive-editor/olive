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

ShakeEffect::ShakeEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_SHAKE_EFFECT), inside(false) {
	EffectRow* intensity_row = add_row("Intensity:");
	intensity_val = intensity_row->add_field(EFFECT_FIELD_DOUBLE);
	intensity_val->set_double_minimum_value(0);

	EffectRow* rotation_row = add_row("Rotation:");
	rotation_val = rotation_row->add_field(EFFECT_FIELD_DOUBLE);
	rotation_val->set_double_minimum_value(0);

	EffectRow* frequency_row = add_row("Frequency:");
	frequency_val = frequency_row->add_field(EFFECT_FIELD_DOUBLE);
	frequency_val->set_double_minimum_value(0);

    // set defaults
	intensity_val->set_double_default_value(50);
	rotation_val->set_double_default_value(0);
	frequency_val->set_double_default_value(10);

	refresh();

	connect(intensity_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(rotation_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(frequency_val, SIGNAL(changed()), this, SLOT(field_changed()));
}

void ShakeEffect::refresh() {
    if (parent_clip->sequence != NULL) {
		shake_limit = qRound(parent_clip->sequence->frame_rate / frequency_val->get_double_value());
        shake_progress = shake_limit;
        next_x = 0;
        next_y = 0;
        next_rot = 0;
    }
}

Effect* ShakeEffect::copy(Clip* c) {
	ShakeEffect* e = new ShakeEffect(c);
	e->intensity_val->set_double_value(intensity_val->get_double_value());
	e->rotation_val->set_double_value(rotation_val->get_double_value());
	e->frequency_val->set_double_value(frequency_val->get_double_value());
	return e;
}

void ShakeEffect::process_gl(int*, int*) {
    if (shake_progress > shake_limit) {
		double ival = intensity_val->get_double_value();
		if ((int)ival > 0) {
            prev_x = next_x;
            prev_y = next_y;
			next_x = (qrand() % (int) (ival * 2)) - ival;
			next_y = (qrand() % (int) (ival * 2)) - ival;

            // find perpendicular slope that passes through (mid_x, mid_y)
            int mid_x = (prev_x + next_x) / 2;
            int mid_y = (prev_y + next_y) / 2;
            int slope_num = (next_y-prev_y);
            if (slope_num > 0) {
                int slope_den = (next_x-prev_x);
                int add = (next_x-prev_x)/4;
                if (inside) add = -add;
                inside = !inside;
                perp_x = mid_x + add;
                perp_y = (-(slope_den / slope_num)) * perp_x + mid_y;
            } else {
                perp_x = mid_x;
                perp_y = mid_y;
            }
        } else {
            prev_x = 0;
            prev_y = 0;
            next_x = 0;
            next_y = 0;
            offset_x = 0;
            offset_y = 0;
        }
		double rot_val = rotation_val->get_double_value();
		if ((int)rot_val > 0) {
            prev_rot = next_rot;
			next_rot = (qrand() % (int) (rot_val * 2)) - rot_val;
        } else {
            prev_rot = 0;
            next_rot = 0;
            offset_rot = 0;
        }
        shake_progress = 0;
    }

    t = (double) shake_progress / (double) shake_limit;

    double oneminust = 1 - t;

    offset_x = (qPow(oneminust, 2)*prev_x) + (2*oneminust*t*perp_x) + (qPow(t, 2) * next_x);
    offset_y = (qPow(oneminust, 2)*prev_y) + (2*oneminust*t*perp_y) + (qPow(t, 2) * next_y);

    offset_rot = lerp(prev_rot, next_rot, t);

    glTranslatef(offset_x, offset_y, 0);
    glRotatef(offset_rot, 0, 0, 1);
    shake_progress++;
}

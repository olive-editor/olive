#include "cornerpineffect.h"

CornerPinEffect::CornerPinEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
    enable_coords = true;

    EffectRow* top_left = add_row("Top Left:");
    top_left_x = top_left->add_field(EFFECT_FIELD_DOUBLE);
    top_left_y = top_left->add_field(EFFECT_FIELD_DOUBLE);

    EffectRow* top_right = add_row("Top Right:");
    top_right_x = top_right->add_field(EFFECT_FIELD_DOUBLE);
    top_right_y = top_right->add_field(EFFECT_FIELD_DOUBLE);

    EffectRow* bottom_left = add_row("Bottom Left:");
    bottom_left_x = bottom_left->add_field(EFFECT_FIELD_DOUBLE);
    bottom_left_y = bottom_left->add_field(EFFECT_FIELD_DOUBLE);

    EffectRow* bottom_right = add_row("Bottom Right:");
    bottom_right_x = bottom_right->add_field(EFFECT_FIELD_DOUBLE);
    bottom_right_y = bottom_right->add_field(EFFECT_FIELD_DOUBLE);

	connect(top_left_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(top_left_y, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(top_right_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(top_right_y, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(bottom_left_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(bottom_left_y, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(bottom_right_x, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(bottom_right_y, SIGNAL(changed()), this, SLOT(field_changed()));
}

void CornerPinEffect::process_coords(double timecode, GLTextureCoords &coords) {
    coords.vertexTopLeftX += top_left_x->get_double_value(timecode);
    coords.vertexTopLeftY += top_left_y->get_double_value(timecode);

    coords.vertexTopRightX += top_right_x->get_double_value(timecode);
    coords.vertexTopRightY += top_right_y->get_double_value(timecode);

    coords.vertexBottomLeftX += bottom_left_x->get_double_value(timecode);
    coords.vertexBottomLeftY += bottom_left_y->get_double_value(timecode);

    coords.vertexBottomRightX += bottom_right_x->get_double_value(timecode);
    coords.vertexBottomRightY += bottom_right_y->get_double_value(timecode);
}

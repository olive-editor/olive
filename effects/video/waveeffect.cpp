#include "waveeffect.h"

WaveEffect::WaveEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_WAVE_EFFECT) {
	enable_shader = true;

	frequency_val = add_row("Frequency:")->add_field(EFFECT_FIELD_DOUBLE);
	frequency_val->set_double_minimum_value(0);
	frequency_val->set_double_default_value(10);

	intensity_val = add_row("Intensity:")->add_field(EFFECT_FIELD_DOUBLE);
	intensity_val->set_double_default_value(10);

	evolution_val = add_row("Evolution:")->add_field(EFFECT_FIELD_DOUBLE);
	evolution_val->set_double_default_value(0);

	connect(frequency_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(intensity_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(evolution_val, SIGNAL(changed()), this, SLOT(field_changed()));

	vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/waveeffect.frag";
}

void WaveEffect::process_shader(double timecode) {
	glslProgram->setUniformValue("frequency", (GLfloat) (frequency_val->get_double_value(timecode)));
	glslProgram->setUniformValue("intensity", (GLfloat) (intensity_val->get_double_value(timecode)*0.01));
	glslProgram->setUniformValue("evolution", (GLfloat) (evolution_val->get_double_value(timecode)*0.01));
}

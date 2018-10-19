#include "temperatureeffect.h"

TemperatureEffect::TemperatureEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_TEMPERATURE_EFFECT) {
    enable_shader = true;

    temp_val = add_row("Temperature:")->add_field(EFFECT_FIELD_DOUBLE);
    temp_val->set_double_minimum_value(2000);
    temp_val->set_double_default_value(5200);
    temp_val->set_double_maximum_value(40000);

    connect(temp_val, SIGNAL(changed()), this, SLOT(field_changed()));

    vertPath = ":/shaders/common.vert";
    fragPath = ":/shaders/temperatureeffect.frag";
}

void TemperatureEffect::process_shader(double timecode) {
    glslProgram->setUniformValue("temperature", (GLfloat) (temp_val->get_double_value(timecode)*0.01));
}

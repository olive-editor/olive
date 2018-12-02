#include "effectgizmo.h"

#include "ui/labelslider.h"
#include "effectfield.h"

EffectGizmo::EffectGizmo(int type) :
    x_field(NULL),
    x_field_multi(1.0),
    y_field(NULL),
    y_field_multi(1.0),
    type(type),
    cursor(-1)
{
    int point_count = (type == GIZMO_TYPE_POLY) ? 4 : 1;
    world_pos.resize(point_count);
    screen_pos.resize(point_count);

    color = Qt::white;
}

void EffectGizmo::set_previous_value() {
    if (x_field != NULL) static_cast<LabelSlider*>(x_field->ui_element)->set_previous_value();
    if (y_field != NULL) static_cast<LabelSlider*>(y_field->ui_element)->set_previous_value();
}

int EffectGizmo::get_point_count() {
    return world_pos.size();
}

int EffectGizmo::get_type() {
    return type;
}

int EffectGizmo::get_cursor() {
    return cursor;
}

void EffectGizmo::set_cursor(int c) {
    cursor = c;
}

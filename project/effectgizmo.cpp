#include "effectgizmo.h"

#include "ui/labelslider.h"
#include "effectfield.h"

EffectGizmo::EffectGizmo(int type) :
    x_field1(NULL),
    x_field_multi1(1.0),
    y_field1(NULL),
    y_field_multi1(1.0),
    x_field2(NULL),
    x_field_multi2(1.0),
    y_field2(NULL),
    y_field_multi2(1.0),
    type(type),
    cursor(-1)
{
    int point_count = (type == GIZMO_TYPE_POLY) ? 4 : 1;
    world_pos.resize(point_count);
    screen_pos.resize(point_count);

    color = Qt::white;
}

void EffectGizmo::set_previous_value() {
    if (x_field1 != NULL) static_cast<LabelSlider*>(x_field1->ui_element)->set_previous_value();
    if (y_field1 != NULL) static_cast<LabelSlider*>(y_field1->ui_element)->set_previous_value();
    if (x_field2 != NULL) static_cast<LabelSlider*>(x_field2->ui_element)->set_previous_value();
    if (y_field2 != NULL) static_cast<LabelSlider*>(y_field2->ui_element)->set_previous_value();
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

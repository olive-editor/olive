#include "effectgizmo.h"

EffectGizmo::EffectGizmo(int type) :
    type(type),
    cursor(-1)
{
    int point_count = (type == GIZMO_TYPE_POLY) ? 4 : 1;
    world_pos.resize(point_count);
    screen_pos.resize(point_count);

    color = Qt::white;
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

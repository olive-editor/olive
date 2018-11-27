#include "effectgizmo.h"

EffectGizmo::EffectGizmo(int type) :
    type(type),
    cursor(-1)
{}

void EffectGizmo::set_pos(int ix, int iy) {
    x = ix;
    y = iy;
}

int EffectGizmo::get_x() {
    return x;
}

int EffectGizmo::get_y() {
    return y;
}

int EffectGizmo::get_type() {
    return type;
}

void EffectGizmo::set_screen_pos(int isx, int isy) {
    sx = isx;
    sy = isy;
}

int EffectGizmo::get_screen_x() {
    return sx;
}

int EffectGizmo::get_screen_y() {
    return sy;
}

int EffectGizmo::get_cursor() {
    return cursor;
}

void EffectGizmo::set_cursor(int c) {
    cursor = c;
}

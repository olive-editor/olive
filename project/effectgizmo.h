#ifndef EFFECTGIZMO_H
#define EFFECTGIZMO_H

#define GIZMO_TYPE_DOT 0
#define GIZMO_TYPE_QUAD 1

#define GIZMO_DOT_SIZE 2.5F

#include <QString>
#include <QRect>

class EffectGizmo
{
public:
    EffectGizmo(int type);

    void set_pos(int ix, int iy);
    int get_x();
    int get_y();
    int get_type();

    void set_screen_pos(int isx, int isy);
    int get_screen_x();
    int get_screen_y();

    int get_cursor();
    void set_cursor(int c);
private:
    int type;
    int x;
    int y;
    int sx;
    int sy;
    int cursor;
};

#endif // EFFECTGIZMO_H

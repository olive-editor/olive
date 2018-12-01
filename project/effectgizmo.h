#ifndef EFFECTGIZMO_H
#define EFFECTGIZMO_H

#define GIZMO_TYPE_DOT 0
#define GIZMO_TYPE_POLY 1

#define GIZMO_DOT_SIZE 2.5F

#include <QString>
#include <QRect>
#include <QPoint>
#include <QVector>
#include <QColor>

class EffectGizmo
{
public:
    EffectGizmo(int type);

    QVector<QPoint> world_pos;
    QVector<QPoint> screen_pos;
    QColor color;
    int get_point_count();

    int get_type();

    int get_cursor();
    void set_cursor(int c);
private:
    int type;
    int cursor;
};

#endif // EFFECTGIZMO_H

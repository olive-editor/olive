/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef EFFECTGIZMO_H
#define EFFECTGIZMO_H

enum GizmoType {
  GIZMO_TYPE_DOT,
  GIZMO_TYPE_POLY,
  GIZMO_TYPE_TARGET
};

#define GIZMO_DOT_SIZE 2.5
#define GIZMO_TARGET_SIZE 5.0

#include <QObject>
#include <QString>
#include <QRect>
#include <QPoint>
#include <QVector>
#include <QColor>

class DoubleField;
class Effect;

class EffectGizmo : public QObject {
  Q_OBJECT
public:
  EffectGizmo(Effect* parent, int type);

  QVector<QVector3D> world_pos;
  QVector<QPoint> screen_pos;

  DoubleField* x_field1;
  double x_field_multi1;
  DoubleField* y_field1;
  double y_field_multi1;
  DoubleField* x_field2;
  double x_field_multi2;
  DoubleField* y_field2;
  double y_field_multi2;

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

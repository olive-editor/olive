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

#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "effects/effect.h"

#include <QImage>

class SolidEffect : public Effect {
  Q_OBJECT
public:
  enum SolidType {
    SOLID_TYPE_COLOR,
    SOLID_TYPE_BARS,
    SOLID_TYPE_CHECKERBOARD
  };

  SolidEffect(Clip* c, const EffectMeta *em);
  virtual void redraw(double timecode);

  void SetType(SolidType type);
private slots:
  void ui_update(const QVariant &d);
private:
  ComboField* solid_type;
  ColorField* solid_color_field;
  DoubleField* opacity_field;
  DoubleField* checkerboard_size_field;
};

#endif // SOLIDEFFECT_H

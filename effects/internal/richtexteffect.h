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

#ifndef RICHTEXTEFFECT_H
#define RICHTEXTEFFECT_H

#include "effects/effect.h"

class RichTextEffect : public Effect {
  Q_OBJECT
public:
  RichTextEffect(Clip* c, const EffectMeta *em);
  virtual void redraw(double timecode) override;
protected:
  virtual bool AlwaysUpdate() override;
private:
  StringField* text_val;
  DoubleField* padding_field;
  DoubleField* position_x;
  DoubleField* position_y;
  ComboField* vertical_align;
  ComboField* autoscroll;

  BoolField* shadow_bool;
  DoubleField* shadow_angle;
  DoubleField* shadow_distance;
  ColorField* shadow_color;
  DoubleField* shadow_softness;
  DoubleField* shadow_opacity;
};

#endif // RICHTEXTEFFECT_H

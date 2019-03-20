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

#ifndef TEXTEFFECT_H
#define TEXTEFFECT_H

#include "effects/effect.h"

#include <QFont>
#include <QImage>

class TextEffect : public Effect {
  Q_OBJECT
public:
  TextEffect(Clip* c, const EffectMeta *em);
  void redraw(double timecode);
private slots:
  void outline_enable(bool);
  void shadow_enable(bool);
private:
  QFont font;

  StringField* text_val;
  DoubleField* size_val;
  ColorField* set_color_button;
  FontField* set_font_combobox;
  ComboField* halign_field;
  ComboField* valign_field;
  BoolField* word_wrap_field;
  DoubleField* padding_field;
  DoubleField* position_x;
  DoubleField* position_y;

  BoolField* outline_bool;
  DoubleField* outline_width;
  ColorField* outline_color;

  BoolField* shadow_bool;
  DoubleField* shadow_angle;
  DoubleField* shadow_distance;
  ColorField* shadow_color;
  DoubleField* shadow_softness;
  DoubleField* shadow_opacity;

};

#endif // TEXTEFFECT_H

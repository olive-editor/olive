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

#include "nodes/node.h"

#include <QFont>
#include <QImage>

class TextEffect : public Node {
  Q_OBJECT
public:
  TextEffect(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual NodePtr Create(Clip *c) override;

  virtual void redraw(double timecode) override;
private slots:
  void outline_enable(bool);
  void shadow_enable(bool);
private:
  QFont font;

  StringInput* text_val;
  DoubleInput* size_val;
  ColorInput* set_color_button;
  FontInput* set_font_combobox;
  ComboInput* halign_field;
  ComboInput* valign_field;
  BoolInput* word_wrap_field;
  DoubleInput* padding_field;
  Vec2Input* position;

  BoolInput* outline_bool;
  DoubleInput* outline_width;
  ColorInput* outline_color;

  BoolInput* shadow_bool;
  DoubleInput* shadow_angle;
  DoubleInput* shadow_distance;
  ColorInput* shadow_color;
  DoubleInput* shadow_softness;
  DoubleInput* shadow_opacity;

};

#endif // TEXTEFFECT_H

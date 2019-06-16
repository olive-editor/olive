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

#include "nodes/oldeffectnode.h"

class RichTextEffect : public OldEffectNode {
  Q_OBJECT
public:
  RichTextEffect(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

  virtual void redraw(const rational& timecode) override;

protected:
  virtual bool AlwaysUpdate() override;
private:
  StringInput* text_val;
  DoubleInput* padding_field;
  Vec2Input* position;
  ComboInput* vertical_align;
  ComboInput* autoscroll;

  BoolInput* shadow_bool;
  DoubleInput* shadow_angle;
  DoubleInput* shadow_distance;
  ColorInput* shadow_color;
  DoubleInput* shadow_softness;
  DoubleInput* shadow_opacity;
};

#endif // RICHTEXTEFFECT_H

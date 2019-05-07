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

#ifndef TIMECODEEFFECT_H
#define TIMECODEEFFECT_H

#include "nodes/oldeffectnode.h"

#include <QFont>
#include <QImage>

class TimecodeEffect : public OldEffectNode {
  Q_OBJECT
public:
  TimecodeEffect(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

  virtual void redraw(double timecode) override;
  DoubleInput* scale_val;
  ColorInput* color_val;
  ColorInput* color_bg_val;
  DoubleInput* bg_alpha;
  Vec2Input* offset_val;
  StringInput* prepend_text;
  ComboInput* tc_select;

protected:
  virtual bool AlwaysUpdate() override;

private:
  QFont font;
  QString display_timecode;
};

#endif // TIMECODEEFFECT_H

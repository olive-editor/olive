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

#include "project/effect.h"

#include <QFont>
#include <QImage>

class TimecodeEffect : public Effect {
  Q_OBJECT
public:
    TimecodeEffect(Clip* c, const EffectMeta *em);
    void redraw(double timecode);
    DoubleField* scale_val;
    ColorField* color_val;
    ColorField* color_bg_val;
    DoubleField* bg_alpha;
    DoubleField* offset_x_val;
    DoubleField* offset_y_val;
    StringField* prepend_text;
    ComboField* tc_select;

private:
    QFont font;
    QString display_timecode;
};

#endif // TIMECODEEFFECT_H

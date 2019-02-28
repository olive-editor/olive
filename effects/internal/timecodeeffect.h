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
    EffectField * scale_val;
    EffectField * color_val;
    EffectField * color_bg_val;
    EffectField * bg_alpha;
    EffectField * offset_x_val;
    EffectField * offset_y_val;
    EffectField * prepend_text;
    EffectField * tc_select;

private:
    QFont font;
    QString display_timecode;
};

#endif // TIMECODEEFFECT_H

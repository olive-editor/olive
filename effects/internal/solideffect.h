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

#include "project/effect.h"

#include <QImage>

class SolidEffect : public Effect {
	Q_OBJECT
public:
    SolidEffect(Clip* c, const EffectMeta *em);
	void redraw(double timecode);
private slots:
    void ui_update(int);
private:
    EffectField* solid_type;
    EffectField* solid_color_field;
    EffectField* opacity_field;
    EffectField* checkerboard_size_field;
};

#endif // SOLIDEFFECT_H

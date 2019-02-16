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

#ifndef CUBETRANSITION_H
#define CUBETRANSITION_H

#include "project/transition.h"

class CubeTransition : public Transition {
public:
    CubeTransition(Clip* c, Clip* s, const EffectMeta* em);
    void process_coords(double timecode, GLTextureCoords &, int data);
};

#endif // CUBETRANSITION_H

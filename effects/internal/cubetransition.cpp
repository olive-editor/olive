/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "cubetransition.h"

#include "debug.h"

CubeTransition::CubeTransition(Clip* c, Clip* s, const EffectMeta* em) : Transition(c, s, em) {
    enable_coords = true;
}

void CubeTransition::process_coords(double progress, GLTextureCoords& coords, int data) {

    coords.vertexTopLeftZ = 1;
    coords.vertexBottomLeftZ = 1;
}

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

#ifndef KEYFRAMEDRAWING_H
#define KEYFRAMEDRAWING_H

#include <QPainter>

#define KEYFRAME_SIZE 6
#define KEYFRAME_COLOR 160

class NodeIO;

void draw_keyframe(QPainter &p, int type, int x, int y, bool darker, int r = KEYFRAME_COLOR, int g = KEYFRAME_COLOR, int b = KEYFRAME_COLOR);
long adjust_row_keyframe(NodeIO* row, long time, long visible_in);

#endif // KEYFRAMEDRAWING_H

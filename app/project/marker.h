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

#ifndef MARKER_H
#define MARKER_H

#define MARKER_SIZE 4

#include <QString>
#include <QPainter>

struct Sequence;

struct Marker {
    long frame;
    QString name;
};

void draw_marker(QPainter& p, int x, int y, int bottom, bool selected);

void set_marker_internal(Sequence* seq, const QVector<int>& clips);
void set_marker_internal(Sequence* seq);

#endif // MARKER_H

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

#include "cursors.h"

#include <QPixmap>
#include <QCursor>

#include <QDebug>

QCursor Olive::Cursor_LeftTrim;
QCursor Olive::Cursor_RightTrim;

QCursor load_cursor(const QString& file, int hotX, int hotY, const bool& right_aligned){
    // load specified file into a pixmap
    QPixmap temp(file);

    // set cursor's horizontal hotspot
    if (right_aligned) {
        hotX = temp.width() - hotX;
    }

    // return cursor
    return QCursor(temp, hotX, hotY);
}

void init_custom_cursors(){
    qInfo() << "Initializing custom cursors";
    Olive::Cursor_LeftTrim = load_cursor(":/cursors/left_side.png", 0, -1, false);
    Olive::Cursor_RightTrim = load_cursor(":/cursors/right_side.png", 0, -1, true);
    qInfo() << "Finished initializing custom cursors";
}

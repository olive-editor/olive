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

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainter>
#include <QCursor>
#include <QDebug>

QCursor olive::Cursor_LeftTrim;
QCursor olive::Cursor_RightTrim;
QCursor olive::Cursor_LeftRipple;
QCursor olive::Cursor_RightRipple;
QCursor olive::Cursor_Slip;
QCursor olive::Cursor_Razor;

const int cursor_size = 24;

QCursor load_cursor(const QString& file, int hotX = -1, int hotY = -1, bool right_aligned = false){
  // load specified file into a pixmap
  QSvgRenderer renderer(file);

  int cursor_size_dpiaware = cursor_size * QApplication::desktop()->devicePixelRatio();
  QPixmap pixmap(cursor_size_dpiaware, cursor_size_dpiaware);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  renderer.render(&painter, pixmap.rect());

  // set cursor's horizontal hotspot
  if (right_aligned) {
    hotX = pixmap.width() - hotX;
  }

  // return cursor
  return QCursor(pixmap, hotX, hotY);
}

void init_custom_cursors(){
  qInfo() << "Initializing custom cursors";
  olive::Cursor_LeftTrim = load_cursor(":/cursors/1_left.svg");
  olive::Cursor_RightTrim = load_cursor(":/cursors/1_right.svg");
  olive::Cursor_LeftRipple = load_cursor(":/cursors/3_left.svg");
  olive::Cursor_RightRipple = load_cursor(":/cursors/3_right.svg");
  olive::Cursor_Slip = load_cursor(":/cursors/4.svg");
  olive::Cursor_Razor = load_cursor(":/cursors/5.svg");
  qInfo() << "Finished initializing custom cursors";
}

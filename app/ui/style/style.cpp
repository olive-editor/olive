/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "style.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <QTextStream>

void olive::style::AppSetDefault()
{
  // We always use Qt Fusion since it's extremely customizable and cross platform
  qApp->setStyle(QStyleFactory::create("Fusion"));

  // Set CSS style for this
  QFile css_file(":/css/olive-dark.css");

  if (css_file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream css_ts(&css_file);

    qApp->setStyleSheet(css_ts.readAll());
  }
}

void olive::style::WidgetSetNative(QWidget *w)
{
#ifdef Q_OS_WIN
  w->setStyleSheet("");
  w->setPalette(w->style()->standardPalette());
  w->setStyleSheet(QString());
  w->setStyle(QStyleFactory::create("windowsvista"));
#else
  Q_UNUSED(w)
#endif
}

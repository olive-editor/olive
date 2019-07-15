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
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>

#include "ui/icons/icons.h"

void olive::style::AppSetDefault()
{
  SetOliveStyle("olive-light");
}

void olive::style::SetOliveStyle(const QString &style_name)
{
  // Set CSS style for this
  QFile css_file(QString(":/css/%1.css").arg(style_name));

  if (css_file.open(QFile::ReadOnly | QFile::Text)) {
    // Read in entire CSS from file and set as the application stylesheet
    QTextStream css_ts(&css_file);

    qApp->setStyleSheet(css_ts.readAll());

    css_file.close();
  }

  // Load all icons for this style
  olive::icon::LoadAll(style_name);
}

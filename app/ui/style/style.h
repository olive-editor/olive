/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QSettings>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class StyleManager : public QObject {
public:
  static void Init();

  static const QString& GetStyle();

  static void SetStyle(const QString& style_path);

  static void UseOSNativeStyling(QWidget* widget);

  static const char* kDefaultStyle;

  static const QMap<QString, QString>& available_themes()
  {
    return available_themes_;
  }

private:
  static QPalette ParsePalette(const QString& ini_path);

  static void ParsePaletteGroup(QSettings* ini, QPalette* palette, QPalette::ColorGroup group);

  static void ParsePaletteColor(QSettings* ini, QPalette* palette, QPalette::ColorGroup group, const QString& role_name);

  static QString current_style_;

  static QMap<QString, QString> available_themes_;

};

OLIVE_NAMESPACE_EXIT

#endif // STYLEMANAGER_H

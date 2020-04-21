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

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QSettings>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class StyleDescriptor {
public:
  StyleDescriptor(const QString& name, const QString& path);

  const QString& name() const;
  const QString& path() const;

private:
  QString name_;
  QString path_;
};

class StyleManager : public QObject {
public:
  static StyleDescriptor DefaultStyle();

  static const QString& GetStyle();

  static void SetStyleFromConfig();

  static void SetStyle(const StyleDescriptor& style);

  static void SetStyle(const QString& style_path);

  static QList<StyleDescriptor> ListInternal();

#ifdef Q_OS_WINDOWS
  static void UseNativeWindowsStyling(QWidget* widget);
#endif

private:
  static QPalette ParsePalette(const QString& ini_path);

  static void ParsePaletteGroup(QSettings* ini, QPalette* palette, QPalette::ColorGroup group);

  static void ParsePaletteColor(QSettings* ini, QPalette* palette, QPalette::ColorGroup group, const QString& role_name);

  static QString current_style_;

};

OLIVE_NAMESPACE_EXIT

#endif // STYLEMANAGER_H

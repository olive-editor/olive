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
#include <QFileInfo>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>

#include "config/config.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

QString StyleManager::current_style_;

QList<StyleDescriptor> StyleManager::ListInternal()
{
  QList<StyleDescriptor> style_list;

  style_list.append(StyleDescriptor(tr("Olive Dark"), ":/style/olive-dark"));
  style_list.append(StyleDescriptor(tr("Olive Light"), ":/style/olive-light"));

  return style_list;
}

void StyleManager::UseOSNativeStyling(QWidget *widget)
{
#if defined(Q_OS_WINDOWS)
  QStyle* s = QStyleFactory::create(QStringLiteral("windowsvista"));
  widget->setStyle(s);
  widget->setPalette(s->standardPalette());
#elif defined(Q_OS_MAC)
  QStyle* s = QStyleFactory::create(QStringLiteral("macintosh"));
  widget->setStyle(s);
  widget->setPalette(s->standardPalette());
#endif
}

QPalette StyleManager::ParsePalette(const QString& ini_path)
{
  QSettings ini(ini_path, QSettings::IniFormat);
  QPalette palette;

  ParsePaletteGroup(&ini, &palette, QPalette::All);
  ParsePaletteGroup(&ini, &palette, QPalette::Active);
  ParsePaletteGroup(&ini, &palette, QPalette::Inactive);
  ParsePaletteGroup(&ini, &palette, QPalette::Disabled);

  return palette;
}

void StyleManager::ParsePaletteGroup(QSettings *ini, QPalette *palette, QPalette::ColorGroup group)
{
  QString group_name;

  switch (group) {
  case QPalette::All:
    group_name = "All";
    break;
  case QPalette::Active:
    group_name = "Active";
    break;
  case QPalette::Inactive:
    group_name = "Inactive";
    break;
  case QPalette::Disabled:
    group_name = "Disabled";
    break;
  default:
    return;
  }

  ini->beginGroup(group_name);

  QStringList keys = ini->childKeys();
  foreach (QString k, keys) {
    ParsePaletteColor(ini, palette, group, k);
  }

  ini->endGroup();
}

void StyleManager::ParsePaletteColor(QSettings *ini, QPalette *palette, QPalette::ColorGroup group, const QString &role_name)
{
  QPalette::ColorRole role;

  if (!QString::compare(role_name, "Window", Qt::CaseInsensitive)) {
    role = QPalette::Window;
  } else if (!QString::compare(role_name, "WindowText", Qt::CaseInsensitive)) {
    role = QPalette::WindowText;
  } else if (!QString::compare(role_name, "Base", Qt::CaseInsensitive)) {
    role = QPalette::Base;
  } else if (!QString::compare(role_name, "AlternateBase", Qt::CaseInsensitive)) {
    role = QPalette::AlternateBase;
  } else if (!QString::compare(role_name, "ToolTipBase", Qt::CaseInsensitive)) {
    role = QPalette::ToolTipBase;
  } else if (!QString::compare(role_name, "ToolTipText", Qt::CaseInsensitive)) {
    role = QPalette::ToolTipText;
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  } else if (!QString::compare(role_name, "PlaceholderText", Qt::CaseInsensitive)) {
    role = QPalette::PlaceholderText;
#endif
  } else if (!QString::compare(role_name, "Text", Qt::CaseInsensitive)) {
    role = QPalette::Text;
  } else if (!QString::compare(role_name, "Button", Qt::CaseInsensitive)) {
    role = QPalette::Button;
  } else if (!QString::compare(role_name, "ButtonText", Qt::CaseInsensitive)) {
    role = QPalette::ButtonText;
  } else if (!QString::compare(role_name, "BrightText", Qt::CaseInsensitive)) {
    role = QPalette::BrightText;
  } else if (!QString::compare(role_name, "Highlight", Qt::CaseInsensitive)) {
    role = QPalette::Highlight;
  } else if (!QString::compare(role_name, "HighlightedText", Qt::CaseInsensitive)) {
    role = QPalette::HighlightedText;
  } else if (!QString::compare(role_name, "Link", Qt::CaseInsensitive)) {
    role = QPalette::Link;
  } else if (!QString::compare(role_name, "LinkVisited", Qt::CaseInsensitive)) {
    role = QPalette::LinkVisited;
  } else {
    return;
  }

  palette->setColor(group, role, QColor(ini->value(role_name).toString()));
}

StyleDescriptor StyleManager::DefaultStyle()
{
  return ListInternal().first();
}

const QString &StyleManager::GetStyle()
{
  return current_style_;
}

void StyleManager::SetStyleFromConfig()
{
  QString config_style = Config::Current()["Style"].toString();

  if (config_style.isEmpty()) {
    SetStyle(DefaultStyle());
  } else {
    SetStyle(config_style);
  }
}

void StyleManager::SetStyle(const StyleDescriptor &style)
{
  SetStyle(style.path());
}

void StyleManager::SetStyle(const QString &style_path)
{
  current_style_ = style_path;

  // Load all icons for this style (icons must be loaded first because the style change below triggers the icon change)
  icon::LoadAll(style_path);

  // Set palette for this
  QString palette_file = QStringLiteral("%1/palette.ini").arg(style_path);
  if (QFileInfo::exists(palette_file)) {
    qApp->setPalette(ParsePalette(palette_file));
  } else {
    qApp->setPalette(qApp->style()->standardPalette());
  }

  // Set CSS style for this
  QFile css_file(QStringLiteral("%1/style.css").arg(style_path));

  if (css_file.exists() && css_file.open(QFile::ReadOnly | QFile::Text)) {
    // Read in entire CSS from file and set as the application stylesheet
    QTextStream css_ts(&css_file);

    qApp->setStyleSheet(css_ts.readAll());

    css_file.close();
  } else {
    qApp->setStyleSheet(QString());
  }
}

StyleDescriptor::StyleDescriptor(const QString &name, const QString &path) :
  name_(name),
  path_(path)
{
}

const QString &StyleDescriptor::name() const
{
  return name_;
}

const QString &StyleDescriptor::path() const
{
  return path_;
}

OLIVE_NAMESPACE_EXIT

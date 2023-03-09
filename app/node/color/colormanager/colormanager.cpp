/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "colormanager.h"

#include <QDir>
#include <QStandardPaths>

#include "common/define.h"
#include "common/filefunctions.h"
#include "config/config.h"
#include "core.h"

namespace olive {

#define super Node

OCIO::ConstConfigRcPtr ColorManager::default_config_ = nullptr;

ColorManager::ColorManager(Project *project) :
  QObject(project),
  config_(nullptr)
{
}

void ColorManager::Init()
{
  // Set config to our built-in default
  config_ = GetDefaultConfig();
  SetDefaultInputColorSpace(config_->getCanonicalName(OCIO::ROLE_DEFAULT));
  project()->SetColorReferenceSpace(OCIO::ROLE_SCENE_LINEAR);
}

OCIO::ConstConfigRcPtr ColorManager::GetConfig() const
{
  return config_;
}

OCIO::ConstConfigRcPtr ColorManager::CreateConfigFromFile(const QString &filename)
{
  return OCIO::Config::CreateFromFile(filename.toUtf8());
}

QString ColorManager::GetConfigFilename() const
{
  return project()->GetColorConfigFilename();
}

OCIO::ConstConfigRcPtr ColorManager::GetDefaultConfig()
{
  return default_config_;
}

void ColorManager::SetUpDefaultConfig()
{
  if (!qEnvironmentVariableIsEmpty("OCIO")) {
    // Attempt to set config from "OCIO" environment variable
    try {
      default_config_ = OCIO::Config::CreateFromEnv();

      return;
    } catch (OCIO::Exception& e) {
      qWarning() << "Failed to load config from OCIO environment variable config:" << e.what();
    }
  }

  // Extract OCIO config - kind of hacky, but it'll work
  QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("ocioconf"));

  FileFunctions::CopyDirectory(QStringLiteral(":/ocioconf"),
                               dir,
                               true);

  qDebug() << "Extracting default OCIO config to" << dir;

  default_config_ = CreateConfigFromFile(QDir(dir).filePath(QStringLiteral("config.ocio")));
}

void ColorManager::SetConfigFilename(const QString &filename)
{
  project()->SetColorConfigFilename(filename);
}

QStringList ColorManager::ListAvailableDisplays()
{
  QStringList displays;

  int number_of_displays = config_->getNumDisplays();

  for (int i=0;i<number_of_displays;i++) {
    displays.append(config_->getDisplay(i));
  }

  return displays;
}

QString ColorManager::GetDefaultDisplay()
{
  return config_->getDefaultDisplay();
}

QStringList ColorManager::ListAvailableViews(QString display)
{
  QStringList views;

  int number_of_views = config_->getNumViews(display.toUtf8());

  for (int i=0;i<number_of_views;i++) {
    views.append(config_->getView(display.toUtf8(), i));
  }

  return views;
}

QString ColorManager::GetDefaultView(const QString &display)
{
  return config_->getDefaultView(display.toUtf8());
}

QStringList ColorManager::ListAvailableLooks()
{
  QStringList looks;

  int number_of_looks = config_->getNumLooks();

  for (int i=0;i<number_of_looks;i++) {
    looks.append(config_->getLookNameByIndex(i));
  }

  return looks;
}

QStringList ColorManager::ListAvailableColorspaces() const
{
  return ListAvailableColorspaces(config_);
}

QString ColorManager::GetDefaultInputColorSpace() const
{
  return project()->GetDefaultInputColorSpace();
}

void ColorManager::SetDefaultInputColorSpace(const QString &s)
{
  project()->SetDefaultInputColorSpace(s);
}

QString ColorManager::GetReferenceColorSpace() const
{
  return project()->GetColorReferenceSpace();
}

QString ColorManager::GetCompliantColorSpace(const QString &s)
{
  if (ListAvailableColorspaces().contains(s)) {
    return s;
  } else {
    return GetDefaultInputColorSpace();
  }
}

ColorTransform ColorManager::GetCompliantColorSpace(const ColorTransform &transform, bool force_display)
{
  if (transform.is_display() || force_display) {
    // Get display information
    QString display = transform.display();
    QString view = transform.view();
    QString look = transform.look();

    // Check if display still exists in config
    if (!ListAvailableDisplays().contains(display)) {
      display = GetDefaultDisplay();
    }

    // Check if view still exists in display
    if (!ListAvailableViews(display).contains(view)) {
      view = GetDefaultView(display);
    }

    // Check if looks still exists
    if (!ListAvailableLooks().contains(look)) {
      look.clear();
    }

    return ColorTransform(display, view, look);

  } else {

    QString output = transform.output();

    if (!ListAvailableColorspaces().contains(output)) {
      output = GetDefaultInputColorSpace();
    }

    return ColorTransform(output);

  }
}

QStringList ColorManager::ListAvailableColorspaces(OCIO::ConstConfigRcPtr config)
{
  QStringList spaces;

  if (config) {
    int number_of_colorspaces = config->getNumColorSpaces();

    for (int i=0;i<number_of_colorspaces;i++) {
      spaces.append(config->getColorSpaceNameByIndex(i));
    }
  }

  return spaces;
}

void ColorManager::GetDefaultLumaCoefs(double *rgb) const
{
  config_->getDefaultLumaCoefs(rgb);
}

Project *ColorManager::project() const
{
  return static_cast<Project*>(parent());
}

void ColorManager::UpdateConfigFromFilename()
{
  try {
    QString config_filename = GetConfigFilename();
    QString old_default_cs = GetDefaultInputColorSpace();

    config_ = OCIO::Config::CreateFromFile(config_filename.toUtf8());

    // Set new default colorspace appropriately
    QString new_default = old_default_cs;
    QStringList available_cs = ListAvailableColorspaces();
    for (int i=0; i<available_cs.size(); i++) {
      const QString &c = available_cs.at(i);
      if (c.compare(old_default_cs, Qt::CaseInsensitive)) {
        new_default = c;
        break;
      }
    }
    SetDefaultInputColorSpace(new_default);

    emit ConfigChanged(config_filename);
  } catch (OCIO::Exception&) {}
}

}

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

const QString ColorManager::kConfigFilenameIn = QStringLiteral("config");
const QString ColorManager::kDefaultFloatColorspaceIn = QStringLiteral("default_float_input");
const QString ColorManager::kDefaultByteColorspaceIn = QStringLiteral("default_byte_input");
const QString ColorManager::kReferenceSpaceIn = QStringLiteral("reference_space");

#define super Node

OCIO::ConstConfigRcPtr ColorManager::default_config_ = nullptr;

ColorManager::ColorManager() :
  config_(nullptr)
{
  // Filename input
  AddInput(kConfigFilenameIn, NodeValue::kFile, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Colorspace input
  AddInput(kDefaultFloatColorspaceIn, NodeValue::kCombo, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  AddInput(kDefaultByteColorspaceIn, NodeValue::kCombo, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Default reference space is scene linear
  AddInput(kReferenceSpaceIn, NodeValue::kCombo, 0, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  // Set config to our built-in default
  SetConfig(GetDefaultConfig());

  if (config_->hasRole("default_float")) {
    SetDefaultFloatInputColorSpace(config_->getCanonicalName("default_float"));
  }
  else {
    SetDefaultFloatInputColorSpace(config_->getCanonicalName(OCIO::ROLE_DEFAULT));
  }

  if (config_->hasRole("default_byte")) {
    SetDefaultByteInputColorSpace(config_->getCanonicalName("default_byte"));
  } else {
    SetDefaultByteInputColorSpace(config_->getCanonicalName(OCIO::ROLE_DEFAULT));
  }
}

OCIO::ConstConfigRcPtr ColorManager::GetConfig() const
{
  return config_;
}

OCIO::ConstConfigRcPtr ColorManager::CreateConfigFromFile(const QString &filename)
{
  OCIO_SET_C_LOCALE_FOR_SCOPE;

  return OCIO::Config::CreateFromFile(filename.toUtf8());
}

QString ColorManager::GetConfigFilename() const
{
  return GetStandardValue(kConfigFilenameIn).toString();
}

OCIO::ConstConfigRcPtr ColorManager::GetDefaultConfig()
{
  return default_config_;
}

void ColorManager::SetUpDefaultConfig()
{
  if (!qEnvironmentVariableIsEmpty("OCIO")) {
    // Attempt to set config from "OCIO" environment variable
    if (LoadConfigFromPath(qEnvironmentVariable("OCIO"))) {
      return;
    }
  }

  // Extract OCIO config - kind of hacky, but it'll work
  QString dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("ocioconf"));

  FileFunctions::CopyDirectory(QStringLiteral(":/ocioconf"),
                               dir,
                               true);

  qDebug() << "Extracting default OCIO config to" << dir;

  {
    OCIO_SET_C_LOCALE_FOR_SCOPE;
    default_config_ = CreateConfigFromFile(QDir(dir).filePath(QStringLiteral("config.ocio")));
  }
}

bool ColorManager::LoadConfigFromPath(QString path)
{
  try {
    OCIO_SET_C_LOCALE_FOR_SCOPE;
    default_config_ = OCIO::Config::CreateFromFile(path.toStdString().c_str());

    try {
      default_config_->validate();
      qInfo() << "Succesfully loaded OCIO config from: " << path;
      return true;
    } catch (OCIO::Exception &e) {
      qWarning() << "Failed to validate config from OCIO environment vairable:" << e.what();
    }

  } catch (OCIO::Exception &e) {
    qWarning() << "Failed to load config from OCIO environment variable config:" << e.what();
  }

  qWarning() << "Falling back on internal config.";

  return false;
}

void ColorManager::SetConfigFilename(const QString &filename)
{
  SetStandardValue(kConfigFilenameIn, filename);
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

QString ColorManager::GetDefaultFloatInputColorSpace() const
{
  return ListAvailableColorspaces().at(GetStandardValue(kDefaultFloatColorspaceIn).toInt());
}

void ColorManager::SetDefaultFloatInputColorSpace(const QString &s)
{
  SetStandardValue(kDefaultFloatColorspaceIn, ListAvailableColorspaces().indexOf(s));
}

QString ColorManager::GetDefaultByteInputColorSpace() const
{
  return ListAvailableColorspaces().at(GetStandardValue(kDefaultByteColorspaceIn).toInt());
}

void ColorManager::SetDefaultByteInputColorSpace(const QString &s)
{
  SetStandardValue(kDefaultByteColorspaceIn, ListAvailableColorspaces().indexOf(s));
}

QString ColorManager::GetReferenceColorSpace() const
{
  ReferenceSpace ref_space = static_cast<ReferenceSpace>(GetStandardValue(kReferenceSpaceIn).toInt());

  if (ref_space == kCompositingLog) {
    return OCIO::ROLE_COMPOSITING_LOG;
  } else {
    return OCIO::ROLE_SCENE_LINEAR;
  }
}

QString ColorManager::GetCompliantColorSpace(const QString &s)
{
  if (ListAvailableColorspaces().contains(s)) {
    return s;
  } else {
    return config_->getCanonicalName(OCIO::ROLE_DEFAULT);
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
      output = config_->getCanonicalName(OCIO::ROLE_DEFAULT);
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

OCIO::ColorSpaceMenuHelperRcPtr ColorManager::CreateMenuHelper(OCIO::ConstConfigRcPtr config, QString categories) {
  OCIO::ColorSpaceMenuHelperRcPtr menu_helper;
  auto params = OCIO::ColorSpaceMenuParameters::Create(config);
  params->setAppCategories(categories.toStdString().c_str());
  menu_helper = OCIO::ColorSpaceMenuHelper::Create(params);

  return menu_helper;
}

void ColorManager::Retranslate()
{
  super::Retranslate();

  SetInputName(kConfigFilenameIn, tr("Configuration"));
  SetInputName(kDefaultFloatColorspaceIn, tr("Default Float Input"));
  SetInputName(kDefaultByteColorspaceIn, tr("Default Byte Input"));
  SetInputName(kReferenceSpaceIn, tr("Reference Space"));

  SetComboBoxStrings(kReferenceSpaceIn, {tr("Scene Linear"), tr("Compositing Log")});
  SetInputProperty(kConfigFilenameIn, QStringLiteral("placeholder"), tr("(built-in)"));
}

void ColorManager::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kConfigFilenameIn) {

    try {
      SetConfig(OCIO::Config::CreateFromFile(GetConfigFilename().toUtf8()));
    } catch (OCIO::Exception&) {}

  }
}

ColorManager::SetLocale::SetLocale(const char* new_locale)
{
  old_locale_ = setlocale(LC_NUMERIC, nullptr);
  setlocale(LC_NUMERIC, new_locale);
}

ColorManager::SetLocale::~SetLocale()
{
  setlocale(LC_NUMERIC, old_locale_.toUtf8());
}

void ColorManager::SetConfig(OCIO::ConstConfigRcPtr config)
{
  config_ = config;

  // FIXME Tom:Do we ever see this on the node properties window?
  //SetComboBoxStrings(kDefaultColorspaceIn, ListAvailableColorspaces());
}

}

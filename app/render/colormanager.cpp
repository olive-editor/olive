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

#include "colormanager.h"

#include <QDir>
#include <QFloat16>
#include <QStandardPaths>

#include "common/define.h"
#include "common/filefunctions.h"
#include "config/config.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

OCIO::ConstConfigRcPtr ColorManager::default_config_;

ColorManager::ColorManager()
{
  // Set config to our built-in default
  config_ = GetDefaultConfig();

  // Default input space
  default_input_color_space_ = QStringLiteral("sRGB OETF");

  // Default reference space is scene linear
  reference_space_ = OCIO::ROLE_SCENE_LINEAR;
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

const QString &ColorManager::GetConfigFilename() const
{
  return config_filename_;
}

OCIO::ConstConfigRcPtr ColorManager::GetDefaultConfig()
{
  return default_config_;
}

void ColorManager::SetUpDefaultConfig()
{
  OCIO_SET_C_LOCALE_FOR_SCOPE;

  if (!qgetenv("OCIO").isEmpty()) {
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

void ColorManager::SetConfig(const QString &filename)
{
  if (filename != config_filename_) {
    SetConfigInternal(filename);

    emit ConfigChanged();
  }
}

void ColorManager::SetConfigInternal(const QString &filename)
{
  config_filename_ = filename;

  OCIO::ConstConfigRcPtr cfg;

  if (config_filename_.isEmpty()) {
    cfg = OCIO::GetCurrentConfig();
  } else {
    cfg = OCIO::Config::CreateFromFile(filename.toUtf8());
  }

  config_ = cfg;
}

void ColorManager::SetDefaultInputColorSpaceInternal(const QString &s)
{
  default_input_color_space_ = s;
}

void ColorManager::SetConfigAndDefaultInput(const QString &filename, const QString &s)
{
  bool config_changed = false;
  bool default_input_changed = false;

  if (filename != config_filename_) {
    SetConfigInternal(filename);
    config_changed = true;
  }

  if (default_input_color_space_ != s) {
    SetDefaultInputColorSpaceInternal(s);
    default_input_changed = true;
  }

  if (config_changed) {
    emit ConfigChanged();
  }

  if (default_input_changed) {
    emit DefaultInputColorSpaceChanged();
  }
}

void ColorManager::DisassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kDisassociate, f);
}

void ColorManager::AssociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kAssociate, f);
}

void ColorManager::ReassociateAlpha(FramePtr f)
{
  AssociateAlphaPixFmtFilter(kReassociate, f);
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

QStringList ColorManager::ListAvailableColorspaces()
{
  return ListAvailableColorspaces(config_);
}

const QString &ColorManager::GetDefaultInputColorSpace() const
{
  return default_input_color_space_;
}

void ColorManager::SetDefaultInputColorSpace(const QString &s)
{
  if (default_input_color_space_ != s) {
    SetDefaultInputColorSpaceInternal(s);

    emit DefaultInputColorSpaceChanged();
  }
}

const QString &ColorManager::GetReferenceColorSpace() const
{
  return reference_space_;
}

void ColorManager::SetReferenceColorSpace(const QString &s)
{
  reference_space_ = s;

  emit ConfigChanged();
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

  int number_of_colorspaces = config->getNumColorSpaces();

  for (int i=0;i<number_of_colorspaces;i++) {
    spaces.append(config->getColorSpaceNameByIndex(i));
  }

  return spaces;
}

void ColorManager::GetDefaultLumaCoefs(float *rgb) const
{
  config_->getDefaultLumaCoefs(rgb);
}

Color ColorManager::GetDefaultLumaCoefs() const
{
  Color c;

  // Just a default value, shouldn't be significant
  c.set_alpha(1.0f);

  // The float data in Color lines up with the "rgb" param of this function
  GetDefaultLumaCoefs(c.data());

  return c;
}

ColorManager::OCIOMethod ColorManager::GetOCIOMethodForMode(RenderMode::Mode mode)
{
  return static_cast<OCIOMethod>(Core::GetPreferenceForRenderMode(mode, QStringLiteral("OCIOMethod")).toInt());
}

void ColorManager::SetOCIOMethodForMode(RenderMode::Mode mode, ColorManager::OCIOMethod method)
{
  Core::SetPreferenceForRenderMode(mode, QStringLiteral("OCIOMethod"), method);
}

void ColorManager::AssociateAlphaPixFmtFilter(ColorManager::AlphaAction action, FramePtr f)
{
  int pixel_count = f->width() * f->height() * kRGBAChannels;

  switch (static_cast<PixelFormat::Format>(f->format())) {
  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    qWarning() << "Alpha association functions received an invalid pixel format";
    break;
  case PixelFormat::PIX_FMT_RGBA8:
  case PixelFormat::PIX_FMT_RGBA16U:
    qWarning() << "Alpha association functions only works on float-based pixel formats at this time";
    break;
  case PixelFormat::PIX_FMT_RGBA16F:
  {
    AssociateAlphaInternal<qfloat16>(action, reinterpret_cast<qfloat16*>(f->data()), pixel_count);
    break;
  }
  case PixelFormat::PIX_FMT_RGBA32F:
  {
    AssociateAlphaInternal<float>(action, reinterpret_cast<float*>(f->data()), pixel_count);
    break;
  }
  }
}

template<typename T>
void ColorManager::AssociateAlphaInternal(ColorManager::AlphaAction action, T *data, int pix_count)
{
  for (int i=0;i<pix_count;i+=kRGBAChannels) {
    T alpha = data[i+kRGBChannels];

    if (action == kAssociate || alpha > 0) {
      for (int j=0;j<kRGBChannels;j++) {
        if (action == kDisassociate) {
          data[i+j] /= alpha;
        } else {
          data[i+j] *= alpha;
        }
      }
    }
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

OLIVE_NAMESPACE_EXIT

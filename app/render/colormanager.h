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

#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>

#include "codec/frame.h"
#include "colorprocessor.h"

#define OCIO_SET_C_LOCALE_FOR_SCOPE ColorManager::SetLocale d("C")

OLIVE_NAMESPACE_ENTER

class ColorManager : public QObject
{
  Q_OBJECT
public:
  ColorManager();

  OCIO::ConstConfigRcPtr GetConfig() const;

  static OCIO::ConstConfigRcPtr CreateConfigFromFile(const QString& filename);

  const QString& GetConfigFilename() const;

  static OCIO::ConstConfigRcPtr GetDefaultConfig();

  static void SetUpDefaultConfig();

  void SetConfig(const QString& filename);

  void SetConfigAndDefaultInput(const QString& filename, const QString& s);

  static void DisassociateAlpha(FramePtr f);

  static void AssociateAlpha(FramePtr f);

  static void ReassociateAlpha(FramePtr f);

  QStringList ListAvailableDisplays();

  QString GetDefaultDisplay();

  QStringList ListAvailableViews(QString display);

  QString GetDefaultView(const QString& display);

  QStringList ListAvailableLooks();

  QStringList ListAvailableColorspaces();

  const QString& GetDefaultInputColorSpace() const;

  void SetDefaultInputColorSpace(const QString& s);

  const QString& GetReferenceColorSpace() const;

  void SetReferenceColorSpace(const QString& s);

  QString GetCompliantColorSpace(const QString& s);

  ColorTransform GetCompliantColorSpace(const ColorTransform& transform, bool force_display = false);

  static QStringList ListAvailableColorspaces(OCIO::ConstConfigRcPtr config);

  void GetDefaultLumaCoefs(double *rgb) const;
  Color GetDefaultLumaCoefs() const;

  class SetLocale
  {
  public:
    SetLocale(const char* new_locale);

    ~SetLocale();

  private:
    QString old_locale_;

  };

signals:
  void ConfigChanged();

  void DefaultInputColorSpaceChanged();

private:
  void SetConfigInternal(const QString& filename);

  void SetDefaultInputColorSpaceInternal(const QString& s);

  OCIO::ConstConfigRcPtr config_;

  enum AlphaAction {
    kAssociate,
    kDisassociate,
    kReassociate
  };

  static void AssociateAlphaPixFmtFilter(AlphaAction action, FramePtr f);

  template<typename T>
  static void AssociateAlphaInternal(AlphaAction action, T* data, int pix_count);

  QString config_filename_;

  QString default_input_color_space_;

  QString reference_space_;

  static OCIO::ConstConfigRcPtr default_config_;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORSERVICE_H

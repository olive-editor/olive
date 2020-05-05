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

  QStringList ListAvailableInputColorspaces();

  const QString& GetDefaultInputColorSpace() const;

  void SetDefaultInputColorSpace(const QString& s);

  const QString& GetReferenceColorSpace() const;

  void SetReferenceColorSpace(const QString& s);

  QString GetCompliantColorSpace(const QString& s);

  ColorTransform GetCompliantColorSpace(const ColorTransform& transform, bool force_display = false);

  static QStringList ListAvailableInputColorspaces(OCIO::ConstConfigRcPtr config);

  void GetDefaultLumaCoefs(float* rgb) const;
  Color GetDefaultLumaCoefs() const;

  enum OCIOMethod {
    kOCIOFast,
    kOCIOAccurate
  };

  static OCIOMethod GetOCIOMethodForMode(RenderMode::Mode mode);

  static void SetOCIOMethodForMode(RenderMode::Mode mode, OCIOMethod method);
  
  class SetCLocale
  {
  public:
    SetCLocale()
    {
#ifdef Q_OS_WINDOWS
      // set locale will only change locale on the current thread
      previousThreadConfig = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);

      // get and store current locale
      ssaLocale = setlocale(LC_ALL, NULL);

      // set to "C" locale
      setlocale(LC_ALL, "C");
#else
      // set to C locale, saving the old one (returned from useLocale)
      currentLocale = newlocale(LC_ALL_MASK,"C",NULL);
      oldLocale = uselocale(currentLocale);
#endif
    }

    ~SetCLocale()
    {
#ifdef Q_OS_WINDOWS
      // thread specific
      setlocale(LC_ALL, qPrintable(ssaLocale));

      // set back to global settings]
      _configthreadlocale(previousThreadConfig);
#else
      // restore the previous locale and freeing the created locale
      uselocale(oldLocale);
      freelocale(currentLocale);
#endif
    }

  private:
#ifdef Q_OS_WINDOWS
    QString ssaLocale;
    int previousThreadConfig;
#else
    locale_t oldLocale;
    locale_t currentLocale;
#endif

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

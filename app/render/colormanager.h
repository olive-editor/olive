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

#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>
#include <QMutex>

#include "codec/frame.h"
#include "colorprocessor.h"
#include "node/node.h"

#define OCIO_SET_C_LOCALE_FOR_SCOPE ColorManager::SetLocale d("C")

namespace olive {

class ColorManager : public Node
{
  Q_OBJECT
public:
  ColorManager();

  virtual QString Name() const override
  {
    return tr("Color Manager");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.colormanager");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryColor};
  }

  virtual QString Description() const override
  {
    return tr("Color management configuration for project.");
  }

  virtual Node* copy() const override
  {
    return new ColorManager();
  }

  OCIO::ConstConfigRcPtr GetConfig() const;

  static OCIO::ConstConfigRcPtr CreateConfigFromFile(const QString& filename);

  QString GetConfigFilename() const;

  static OCIO::ConstConfigRcPtr GetDefaultConfig();

  static void SetUpDefaultConfig();

  void SetConfigFilename(const QString& filename);

  QStringList ListAvailableDisplays();

  QString GetDefaultDisplay();

  QStringList ListAvailableViews(QString display);

  QString GetDefaultView(const QString& display);

  QStringList ListAvailableLooks();

  QStringList ListAvailableColorspaces() const;

  QString GetDefaultInputColorSpace() const;

  void SetDefaultInputColorSpace(const QString& s);

  QString GetReferenceColorSpace() const;

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

  QMutex* mutex()
  {
    return &mutex_;
  }

  static const QString kConfigFilenameIn;
  static const QString kDefaultColorspaceIn;
  static const QString kReferenceSpaceIn;

  virtual void Retranslate() override;

protected:
  virtual void InputValueChangedEvent(const QString &input, int element) override;

private:
  enum ReferenceSpace {
    kSceneLinear,
    kCompositingLog
  };

  void SetConfig(OCIO::ConstConfigRcPtr config);

  OCIO::ConstConfigRcPtr config_;

  QMutex mutex_;

  static OCIO::ConstConfigRcPtr default_config_;

};

}

#endif // COLORSERVICE_H

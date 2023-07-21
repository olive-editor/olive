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

#ifndef COLORSERVICE_H
#define COLORSERVICE_H

#include <memory>
#include <QMutex>

#include "codec/frame.h"
#include "node/node.h"
#include "render/colorprocessor.h"

namespace olive {

class ColorManager : public QObject
{
  Q_OBJECT
public:
  ColorManager(Project *project);

  void Init();

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

  Project *project() const;

  void UpdateConfigFromFilename();

signals:
  void ConfigChanged(const QString &s);

  void ReferenceSpaceChanged(const QString &s);

  void DefaultInputChanged(const QString &s);

private:
  OCIO::ConstConfigRcPtr config_;

  static OCIO::ConstConfigRcPtr default_config_;

};

}

#endif // COLORSERVICE_H

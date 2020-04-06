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

  void SetConfig(const QString& filename);

  void SetConfig(OCIO::ConstConfigRcPtr config);

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

  static QStringList ListAvailableInputColorspaces(OCIO::ConstConfigRcPtr config);

  enum OCIOMethod {
    kOCIOFast,
    kOCIOAccurate
  };

  static OCIOMethod GetOCIOMethodForMode(RenderMode::Mode mode);

  static void SetOCIOMethodForMode(RenderMode::Mode mode, OCIOMethod method);

signals:
  void ConfigChanged();

private:
  OCIO::ConstConfigRcPtr config_;

  enum AlphaAction {
    kAssociate,
    kDisassociate,
    kReassociate
  };

  static void AssociateAlphaPixFmtFilter(AlphaAction action, FramePtr f);

  template<typename T>
  static void AssociateAlphaInternal(AlphaAction action, T* data, int pix_count);

  QString default_input_color_space_;

  QString reference_space_;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORSERVICE_H

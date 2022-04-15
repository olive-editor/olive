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

#ifndef OCIOGRADINGTRANSFORMNODE_H
#define OCIOGRADINGTRANSFORMNODE_H

#include "node/color/ociobase/ociobase.h"
#include "render/colorprocessor.h"

namespace olive {

class OCIOGradingTransformNode : public OCIOBaseNode
{
  Q_OBJECT
 public:
  OCIOGradingTransformNode();

  NODE_DEFAULT_DESTRUCTOR(OCIOGradingTransformNode)
  NODE_COPY_FUNCTION(OCIOGradingTransformNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;
  virtual void InputValueChangedEvent(const QString &input, int element) override;

  static const QString kTypeInput;
  static const QString kBrightnessInput;
  static const QString kContrastInput;
  static const QString kGammaInput;
  static const QString kOffsetInput;
  static const QString kExposureInput;
  static const QString kLiftInput;
  static const QString kGainInput;
  static const QString kSaturationInput;
  static const QString kPivotInput;
  static const QString kPivotBlackInput;
  static const QString kPivotWhiteInput;
  static const QString kClampBlackInput;
  static const QString kClampWhiteInput;

protected slots:
  virtual void ConfigChanged() override;

};

} // olive

#endif

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

#include "node/node.h"
#include "render/colorprocessor.h"

namespace olive {
class OCIOGradingTransformNode : public Node {
  Q_OBJECT
 public:
  OCIOGradingTransformNode();

  NODE_DEFAULT_DESTRUCTOR(OCIOGradingTransformNode)

  void GenerateProcessor();

  virtual Node *copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;
  virtual void InputValueChangedEvent(const QString &input, int element) override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;
  virtual void Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const override;

  ColorProcessorPtr GetColorProcessor() { return processor_; };
  OCIO::GpuShaderDescRcPtr GetGPUShaderDesc() { return shader_desc_; };

  static const QString kTextureInput;
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

  const QString shader_text_;

  ColorProcessorPtr processor_;
  OCIO::GpuShaderDescRcPtr shader_desc_;

  QString display_;
  QString view_;
};

} // olive

#endif

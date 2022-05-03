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

#ifndef CHROMAKEYNODE_H
#define CHROMAKEYNODE_H

#include "node/color/ociobase/ociobase.h"

namespace olive {

class ChromaKeyNode : public OCIOBaseNode {
  Q_OBJECT
 public:
  ChromaKeyNode();

  NODE_DEFAULT_DESTRUCTOR(ChromaKeyNode)
  NODE_COPY_FUNCTION(ChromaKeyNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void InputValueChangedEvent(const QString& input, int element) override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;
  virtual void Value(const NodeValueRow& value, const NodeGlobals& globals, NodeValueTable* table) const override;

  virtual void ConfigChanged() override {};

  static const QString kColorInput;
  static const QString kMaskOnlyInput;
  static const QString kUpperToleranceInput;
  static const QString kLowerToleranceInput;
  static const QString kGarbageMatteInput;
  static const QString kCoreMatteInput;
  static const QString kShadowsInput;
  static const QString kHighlightsInput;

private:
  void GenerateProcessor();



};

} // namespace olive

#endif // CHROMAKEYNODE_H

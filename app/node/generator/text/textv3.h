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

#ifndef TEXTGENERATORV3_H
#define TEXTGENERATORV3_H

#include "node/generator/shape/shapenodebase.h"
#include "node/gizmo/text.h"

namespace olive {

class TextGeneratorV3 : public ShapeNodeBase
{
  Q_OBJECT
public:
  TextGeneratorV3();

  NODE_DEFAULT_DESTRUCTOR(TextGeneratorV3)
  NODE_COPY_FUNCTION(TextGeneratorV3)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;

  virtual void UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals) override;

  static const QString kTextInput;

private:
  TextGizmo *text_gizmo_;

};

}

#endif // TEXTGENERATORV3_H

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

#ifndef TEXTGENERATORLEGACY_H
#define TEXTGENERATORLEGACY_H

#include "node/node.h"

namespace olive {

class TextGeneratorLegacy : public Node
{
  Q_OBJECT
public:
  TextGeneratorLegacy();

  NODE_DEFAULT_DESTRUCTOR(TextGeneratorLegacy)
  NODE_COPY_FUNCTION(TextGeneratorLegacy)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;

  static const QString kTextInput;
  static const QString kHtmlInput;
  static const QString kColorInput;
  static const QString kVAlignInput;
  static const QString kFontInput;
  static const QString kFontSizeInput;

};

}

#endif // TEXTGENERATORLEGACY_H

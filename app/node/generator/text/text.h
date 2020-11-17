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

#ifndef TEXTGENERATOR_H
#define TEXTGENERATOR_H

#include "node/node.h"

namespace olive {

class TextGenerator : public Node
{
  Q_OBJECT
public:
  TextGenerator();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual NodeValueTable Value(NodeValueDatabase& value) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;

private:
  NodeInput* text_input_;

  NodeInput* color_input_;

  NodeInput* valign_input_;

  NodeInput* font_input_;

  NodeInput* font_size_input_;

};

}

#endif // TEXTGENERATOR_H

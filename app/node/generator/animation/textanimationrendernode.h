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

#ifndef TEXTANIMATIONRENDERNODE_H
#define TEXTANIMATIONRENDERNODE_H

#include <QPainterPath>

#include "common/bezier.h"
#include "node/node.h"
#include "node/gizmo/point.h"
#include "node/generator/animation/textanimationengine.h"

class QTextCursor;


namespace olive {

class TextAnimationRenderNode : public Node
{
  Q_OBJECT
public:
  TextAnimationRenderNode();

  NODE_DEFAULT_FUNCTIONS(TextAnimationRenderNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;


  static const QString kRichTextInput;
  static const QString kAnimatorsInput;

protected:
  GenerateJob GetGenerateJob(const NodeValueRow &value) const;

private:
  int CalculateLineHeight(QTextCursor& start) const;

private:
   TextAnimationEngine * engine_;
};

}

#endif // TEXTANIMATIONRENDERNODE_H

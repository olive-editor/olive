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

#ifndef TEXTANIMATIONNODE_H
#define TEXTANIMATIONNODE_H

#include "node/node.h"

namespace olive {

/// This node produces an XML string that descirbes the animation of one
/// feature of a rich text. This node has an input that can be connected
/// to the output of another instance of this node. In this case, the produced
/// XML string is appended to such input. This allows multiple effects to be cascaded
class TextAnimationNode : public Node
{
  Q_OBJECT
public:
  TextAnimationNode();

  NODE_DEFAULT_FUNCTIONS(TextAnimationNode)

  virtual QString Name() const override
  {
    return tr("TextAnimation");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.textanimation");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryGenerator};
  }

  virtual QString Description() const override
  {
    return tr("Create a descriptor for a text animation. More animations can be cascaded.");
  }

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;


  // a string that is the output of another instance of this node
  static const QString kCompositeInput;
  // 'kCompositeInput' plus the new animation
  static const QString kCompositeOutput;

  // the feature that is being animated
  static const QString kFeatureInput;
  // index of first character to be animated
  static const QString kIndexFromInput;
  // index of last character to be animated
  static const QString kIndexToInput;
  // number of characters to be skipped for each one that is applied
  static const QString kStrideInput;
  // range [0-1]. When 0 all characters start the animation at the begin;
  // When 1, each character starts the animation when the previous is over
  static const QString kOverlapInInput;
  // range [0-1]. When 0 all characters finish the animation at the end;
  // When 1, each character finishes the animation before the next starts
  static const QString kOverlapOutInput;
  // Kind of curve used for animation
  static const QString kCurveInput;
  // a coefficent that may (or may not) influence the shape of the curve
  static const QString kC1Input;
  // a coefficent that may (or may not) influence the shape of the curve
  static const QString kC2Input;
  // maximum value of the feature that is being animated
  static const QString kValueInput;
  // Evolution of the transition in range [0,1]. When 0, the animated value is equal to 'value';
  // When 1, the animated value is equal to 0.
  static const QString kProgressInput;

};

}

#endif // TEXTANIMATIONNODE_H

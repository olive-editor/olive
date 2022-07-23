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

#include "textanimationnode.h"
#include "textanimation.h"

#include "textanimationxmlformatter.h"

namespace olive {

const QString TextAnimationNode::kCompositeInput = QStringLiteral("composite_in");
const QString TextAnimationNode::kCompositeOutput = QStringLiteral("composite_out");
const QString TextAnimationNode::kFeatureInput = QStringLiteral("feature_in");
const QString TextAnimationNode::kIndexFromInput = QStringLiteral("from_in");
const QString TextAnimationNode::kIndexToInput = QStringLiteral("to_in");
const QString TextAnimationNode::kOverlapInInput = QStringLiteral("overlap_in_in");
const QString TextAnimationNode::kOverlapOutInput = QStringLiteral("overlap_out_in");
const QString TextAnimationNode::kCurveInput = QStringLiteral("curve_in");
const QString TextAnimationNode::kC1Input = QStringLiteral("c1_in");
const QString TextAnimationNode::kC2Input = QStringLiteral("c2_in");
const QString TextAnimationNode::kValueInput = QStringLiteral("fvalue_in");
const QString TextAnimationNode::kAlphaInput = QStringLiteral("alpha_in");

#define super Node

TextAnimationNode::TextAnimationNode()
{
  AddInput( kCompositeInput, NodeValue::kText,
            QStringLiteral("<!-- Don't write here. Attach a TextAnimation node -->"),
            InputFlags(kInputFlagNotKeyframable));
  AddInput( kCompositeOutput, NodeValue::kText, QString(""), InputFlags(kInputFlagHidden));

  AddInput( kFeatureInput, NodeValue::kCombo, 1);
  // keep order defined in TextAnimation::Feature, not the one in QMaps
  SetComboBoxStrings( kFeatureInput, QStringList()
                      << tr("None")
                      << tr("Vertical Position")
                      << tr("Horizontal Position")
                      << tr("Rotation")
                      << tr("Spacing")
                      << tr("Vertical Stretch")
                      << tr("Horizontal Stretch")
                      << tr("Transparency")
                      );

  // index of first character. Should be int, but float is used to allow key-framing over time
  AddInput( kIndexFromInput, NodeValue::kFloat, 0.);
  SetInputProperty( kIndexFromInput, QStringLiteral("min"), 0.);

  // index of last character. Should be int, but float is used to allow key-framing over time
  AddInput( kIndexToInput, NodeValue::kFloat, -1.);  // default to negative value to animate the full text
  SetInputProperty( kIndexToInput, QStringLiteral("min"), -1.);

  AddInput( kOverlapInInput, NodeValue::kFloat, 1.);
  SetInputProperty( kOverlapInInput, QStringLiteral("min"), 0.);
  SetInputProperty( kOverlapInInput, QStringLiteral("max"), 1.);

  AddInput( kOverlapOutInput, NodeValue::kFloat, 1.);
  SetInputProperty( kOverlapOutInput, QStringLiteral("min"), 0.);
  SetInputProperty( kOverlapOutInput, QStringLiteral("max"), 1.);

  AddInput( kCurveInput, NodeValue::kCombo, 0);
  // keep order defined in TextAnimation::Curve, not the one in QMaps
  SetComboBoxStrings( kCurveInput, QStringList()
                      << tr("Step")
                      << tr("Linear")
                      << tr("Ease_Sine")
                      << tr("Ease_Quad")
                      << tr("Ease_Back")
                      << tr("Ease_Elastic")
                      << tr("Ease_Bounce")
                      );

  AddInput( kC1Input, NodeValue::kFloat, 1.0);

  AddInput( kC2Input, NodeValue::kFloat, 1.0);

  AddInput( kValueInput, NodeValue::kFloat, 10.);

  AddInput( kAlphaInput, NodeValue::kFloat, 0.);
  SetInputProperty( kAlphaInput, QStringLiteral("min"), 0.);
  SetInputProperty( kAlphaInput, QStringLiteral("max"), 1.);

   SetEffectInput(kCompositeInput);
}

void TextAnimationNode::Retranslate()
{
  super::Retranslate();

  SetInputName( kCompositeInput, QStringLiteral("Composite"));
  SetInputName( kFeatureInput, QStringLiteral("Feature"));
  SetInputName( kIndexFromInput, QStringLiteral("Index from"));
  SetInputName( kIndexToInput, QStringLiteral("Index to"));
  SetInputName( kOverlapInInput, QStringLiteral("Overlap IN"));
  SetInputName( kOverlapOutInput, QStringLiteral("Overlap OUT"));
  SetInputName( kCurveInput, QStringLiteral("Curve"));
  SetInputName( kC1Input, QStringLiteral("C1"));
  SetInputName( kC2Input, QStringLiteral("C2"));
  SetInputName( kValueInput, QStringLiteral("Value"));
  SetInputName( kAlphaInput, QStringLiteral("Alpha"));
}


void TextAnimationNode::Value(const NodeValueRow &value, const NodeGlobals & /*globals*/, NodeValueTable *table) const
{
  GenerateJob job;
  job.Insert(value);

  // all previous animations
  QString composite = job.Get(kCompositeInput).toString();

  // add the new animation
  TextAnimation::Descriptor animation;

  animation.feature = (TextAnimation::Feature) (job.Get(kFeatureInput).toInt());
  animation.character_from = (int)(job.Get(kIndexFromInput).toDouble());
  animation.character_to = (int)(job.Get(kIndexToInput).toInt());
  animation.overlap_in = job.Get(kOverlapInInput).toDouble ();
  animation.overlap_out = job.Get(kOverlapOutInput).toDouble ();
  animation.curve = (TextAnimation::Curve) (job.Get(kCurveInput).toInt());
  animation.c1 = job.Get(kC1Input).toDouble ();;
  animation.c2 = job.Get(kC2Input).toDouble ();;
  animation.value = job.Get(kValueInput).toDouble ();;
  animation.alpha = job.Get(kAlphaInput).toDouble ();;

  TextAnimationXmlFormatter formatter;
  formatter.SetAnimationData( & animation);

  // append the new animation descriptor to the composite
  job.Insert( kCompositeOutput, NodeValue(NodeValue::kText,
              composite + formatter.Format() + "\n", this));

  // append new to old
  table->Push( job.Get(kCompositeOutput));
}


}  // olive


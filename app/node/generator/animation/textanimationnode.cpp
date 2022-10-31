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
const QString TextAnimationNode::kLastToFirstInput = QStringLiteral("from_last_in");
const QString TextAnimationNode::kStrideInput = QStringLiteral("stride_in");
const QString TextAnimationNode::kOverlapInInput = QStringLiteral("overlap_in_in");
const QString TextAnimationNode::kOverlapOutInput = QStringLiteral("overlap_out_in");
const QString TextAnimationNode::kCurveInput = QStringLiteral("curve_in");
const QString TextAnimationNode::kC1Input = QStringLiteral("c1_in");
const QString TextAnimationNode::kC2Input = QStringLiteral("c2_in");
const QString TextAnimationNode::kValueInput = QStringLiteral("fvalue_in");
const QString TextAnimationNode::kProgressInput = QStringLiteral("progress_in");

#define super Node

TextAnimationNode::TextAnimationNode()
{
  AddInput( kCompositeInput, NodeValue::kText,
            QStringLiteral("<!-- Don't write here. Attach a TextAnimation node -->"),
            InputFlags(kInputFlagNotKeyframable));
  SetInputProperty( kCompositeInput, QString("tooltip"),
                    tr("<p>This input is used to cascade multiple animations.</p>"));

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
  SetInputProperty( kFeatureInput, QString("tooltip"),
                    tr("<p>The aspect of the text that is animated.</p>"));

  // index of first character. Should be int, but float is used to allow key-framing over time
  AddInput( kIndexFromInput, NodeValue::kFloat, 0.);
  SetInputProperty( kIndexFromInput, QStringLiteral("min"), 0.);
  SetInputProperty( kIndexFromInput, QString("tooltip"),
                    tr("<p>The index of the first character to which animation applies.</p>"
                       "<p>Use 0 to start from the first character.</p>"));

  // index of last character. Should be int, but float is used to allow key-framing over time
  AddInput( kIndexToInput, NodeValue::kFloat, -1.);  // default to negative value to animate the full text
  SetInputProperty( kIndexToInput, QStringLiteral("min"), -1.);
  SetInputProperty( kIndexToInput, QString("tooltip"),
                    tr("<p>The index of the last character to which animation applies.</p>"
                       "<p>Use -1 for the last character of the whole text.</p>"));

  // flag that indicates if transition starts from first or last letter
  AddInput( kLastToFirstInput, NodeValue::kBoolean, false);
  SetInputProperty( kLastToFirstInput, QString("tooltip"),
                    tr("<p>When checked, the first character to be animated is the one"
                       " at position 'First character'. Otherwise, the first character to"
                       " be animated is the one at position 'Last character'</p>"));

  // when 0, effect is applied to all characters. When greater then 0, this number of characters is skipped
  // for each one for which the effect is applied
  AddInput( kStrideInput, NodeValue::kFloat, 0.);
  SetInputProperty( kStrideInput, QStringLiteral("min"), 0.);
  SetInputProperty( kStrideInput, QString("tooltip"),
                    tr("<p>When 0, the animation is applied to all characters defined by parameters 'from' and 'to'.</p>"
                       "<p>When greater than 0, this is the number of characters that are skipped for each character "
                       "that is animated</p>"));

  AddInput( kValueInput, NodeValue::kFloat, 250.);
  SetInputProperty( kValueInput, QString("tooltip"),
                    tr("<p>The amount of the feature that is animated when the animation begins,"
                       "i.e. when 'progress' is 0'.</p>"));

  AddInput( kOverlapInInput, NodeValue::kFloat, 0.5);
  SetInputProperty( kOverlapInInput, QStringLiteral("min"), 0.);
  SetInputProperty( kOverlapInInput, QStringLiteral("max"), 1.);
  SetInputProperty( kOverlapInInput, QString("tooltip"),
                    tr("<p><b>Range: 0-1</b></p>"
                       "<p>This controls when each charater starts the animation. "
                       "When 0, all characters start the animation at the same time; "
                       "when 1, the begin of the animation for each character is distributed over time.</p>"));

  AddInput( kOverlapOutInput, NodeValue::kFloat, 0.5);
  SetInputProperty( kOverlapOutInput, QStringLiteral("min"), 0.);
  SetInputProperty( kOverlapOutInput, QStringLiteral("max"), 1.);
  SetInputProperty( kOverlapOutInput, QString("tooltip"),
                    tr("<p><b>Range: 0-1</b></p>"
                       "<p>This controls when each charater ends the animation. "
                       "When 0, all characters end the animation at the same time; "
                       "when 1, the end of the animation for each character is distributed over time.</p>"));

  AddInput( kCurveInput, NodeValue::kCombo, 1);
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
  SetInputProperty( kCurveInput, QString("tooltip"),
                    tr("<p>The shape of the curve used for animation.</p>"));

  AddInput( kC1Input, NodeValue::kFloat, 1.0);
  SetInputProperty( kC1Input, QString("tooltip"),
                    tr("<p>A coefficient that modifies the animation curve.</p>"
                       "<p>According to the kind of curve, this may or may not affect the curve. "
                       "Value 1 is always a good defualt value.</p>"));

  AddInput( kC2Input, NodeValue::kFloat, 1.0);
  SetInputProperty( kC2Input, QString("tooltip"),
                    tr("<p>A coefficient that modifies the animation curve.</p>"
                       "<p>According to the kind of curve, this may or may not affect the curve. "
                       "Value 1 is always a good defualt value.</p>"));

  AddInput( kProgressInput, NodeValue::kFloat, 0.);
  SetInputProperty( kProgressInput, QStringLiteral("min"), 0.);
  SetInputProperty( kProgressInput, QStringLiteral("max"), 1.);
  SetInputProperty( kProgressInput, QString("tooltip"),
                    tr("<p><b>Range: 0-1</b></p>"
                       "<p>The progress of the animation.</p> <p>This input must be animated by keyframes from "
                       "0 to 1 to let animation happen. When 0, the animation is not started; when 1,"
                       "the animation is complete</p>"));

  SetEffectInput(kCompositeInput);
}

void TextAnimationNode::Retranslate()
{
  super::Retranslate();

  SetInputName( kCompositeInput, tr("Animators"));
  SetInputName( kFeatureInput, tr("Feature"));
  SetInputName( kIndexFromInput, tr("First character"));
  SetInputName( kIndexToInput, tr("Last character"));
  SetInputName( kLastToFirstInput, tr("Last to first"));
  SetInputName( kStrideInput, tr("Stride"));
  SetInputName( kOverlapInInput, tr("Start timing"));
  SetInputName( kOverlapOutInput, tr("End timing"));
  SetInputName( kCurveInput, tr("Curve"));
  SetInputName( kC1Input, tr("C1"));
  SetInputName( kC2Input, tr("C2"));
  SetInputName( kValueInput, tr("Value"));
  SetInputName( kProgressInput, tr("Progress"));
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
  animation.character_to = (int)(job.Get(kIndexToInput).toDouble());
  animation.last_to_first = job.Get(kLastToFirstInput).toBool();
  animation.stride = (int)(job.Get(kStrideInput).toDouble());
  animation.overlap_in = job.Get(kOverlapInInput).toDouble ();
  animation.overlap_out = job.Get(kOverlapOutInput).toDouble ();
  animation.curve = (TextAnimation::Curve) (job.Get(kCurveInput).toInt());
  animation.c1 = job.Get(kC1Input).toDouble ();
  animation.c2 = job.Get(kC2Input).toDouble ();
  animation.value = job.Get(kValueInput).toDouble ();
  animation.progress = job.Get(kProgressInput).toDouble ();

  TextAnimationXmlFormatter formatter;
  formatter.SetAnimationData( & animation);

  // append the new animation descriptor to the composite
  job.Insert( kCompositeOutput, NodeValue(NodeValue::kText,
              composite + formatter.Format() + "\n", this));

  // append new to old
  table->Push( job.Get(kCompositeOutput));
}


}  // olive


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

#include "timeremap.h"

#include "widget/slider/rationalslider.h"

namespace olive {

const QString TimeRemapNode::kTimeInput = QStringLiteral("time_in");
const QString TimeRemapNode::kInputInput = QStringLiteral("input_in");

#define super Node

TimeRemapNode::TimeRemapNode()
{
  AddInput(kTimeInput, NodeValue::kRational, QVariant::fromValue(rational(0)), InputFlags(kInputFlagNotConnectable));
  SetInputProperty(kTimeInput, QStringLiteral("view"), RationalSlider::kTime);
  SetInputProperty(kTimeInput, QStringLiteral("viewlock"), true);

  AddInput(kInputInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
}

Node *TimeRemapNode::copy() const
{
  return new TimeRemapNode();
}

QString TimeRemapNode::Name() const
{
  return tr("Time Remap");
}

QString TimeRemapNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.timeremap");
}

QVector<Node::CategoryID> TimeRemapNode::Category() const
{
  return {kCategoryGeneral};
}

QString TimeRemapNode::Description() const
{
  return tr("Arbitrarily remap time through the nodes.");
}

TimeRange TimeRemapNode::InputTimeAdjustment(const QString &input, int element, const TimeRange &input_time) const
{
  if (input == kInputInput) {
    rational target_time = GetRemappedTime(input_time.in());

    return TimeRange(target_time, target_time + input_time.length());
  } else {
    return super::InputTimeAdjustment(input, element, input_time);
  }
}

TimeRange TimeRemapNode::OutputTimeAdjustment(const QString &input, int element, const TimeRange &input_time) const
{
  /*if (input == kInputInput) {
    rational target_time = GetValueAtTime(kTimeInput, input_time.in()).value<rational>();

    return TimeRange(target_time, target_time + input_time.length());
  } else {
    return super::OutputTimeAdjustment(input, element, input_time);
  }*/
  return super::OutputTimeAdjustment(input, element, input_time);
}

void TimeRemapNode::Retranslate()
{
  SetInputName(kTimeInput, QStringLiteral("Time"));
  SetInputName(kInputInput, QStringLiteral("Input"));
}

QVector<QString> TimeRemapNode::inputs_for_output(const QString &output) const
{
  Q_UNUSED(output)
  return {kInputInput};
}

void TimeRemapNode::Hash(const QString &output, QCryptographicHash &hash, const rational &time, const VideoParams &video_params) const
{
  // Don't hash anything of our own, just pass-through to the connected node at the remapped tmie
  Q_UNUSED(output)
  if (IsInputConnected(kInputInput)) {
    NodeOutput out = GetConnectedOutput(kInputInput);
    out.node()->Hash(out.output(), hash, GetRemappedTime(time), video_params);
  }
}

rational TimeRemapNode::GetRemappedTime(const rational &input) const
{
  return GetValueAtTime(kTimeInput, input).value<rational>();
}

}

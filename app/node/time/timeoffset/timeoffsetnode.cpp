/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "timeoffsetnode.h"

#include "widget/slider/rationalslider.h"

namespace olive
{

const QString TimeOffsetNode::kTimeInput = QStringLiteral("time_in");
const QString TimeOffsetNode::kInputInput = QStringLiteral("input_in");

#define super Node

TimeOffsetNode::TimeOffsetNode()
{
	AddInput(kTimeInput, NodeValue::kRational, QVariant::fromValue(rational(0)),
			 InputFlags(kInputFlagNotConnectable));
	SetInputProperty(kTimeInput, QStringLiteral("view"), RationalSlider::kTime);
	SetInputProperty(kTimeInput, QStringLiteral("viewlock"), true);

	AddInput(kInputInput, NodeValue::kNone,
			 InputFlags(kInputFlagNotKeyframable));
}

void TimeOffsetNode::Retranslate()
{
	super::Retranslate();

	SetInputName(kTimeInput, QStringLiteral("Time"));
	SetInputName(kInputInput, QStringLiteral("Input"));
}

TimeRange TimeOffsetNode::InputTimeAdjustment(const QString &input, int element,
											  const TimeRange &input_time,
											  bool clamp) const
{
	if (input == kInputInput) {
		return TimeRange(GetRemappedTime(input_time.in()),
						 GetRemappedTime(input_time.out()));
	} else {
		return super::InputTimeAdjustment(input, element, input_time, clamp);
	}
}

TimeRange
TimeOffsetNode::OutputTimeAdjustment(const QString &input, int element,
									 const TimeRange &input_time) const
{
	if (input == kInputInput) {
		// The inverse of InputTimeAdjustment(): times at the input are mapped
		// back to the output by subtracting the offset again
		return TimeRange(GetRemappedOutputTime(input_time.in()),
						 GetRemappedOutputTime(input_time.out()));
	} else {
		return super::OutputTimeAdjustment(input, element, input_time);
	}
}

void TimeOffsetNode::Value(const NodeValueRow &value,
						   const NodeGlobals &globals,
						   NodeValueTable *table) const
{
	table->Push(value[kInputInput]);
}

rational TimeOffsetNode::GetRemappedTime(const rational &input) const
{
	return input + GetValueAtTime(kTimeInput, input).value<rational>();
}

rational TimeOffsetNode::GetRemappedOutputTime(const rational &input) const
{
	return input - GetValueAtTime(kTimeInput, input).value<rational>();
}

}

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

#include "keyframe.h"

NodeKeyframe::NodeKeyframe(const rational &time, const QVariant &value, const NodeKeyframe::Type &type, const int &track) :
  time_(time),
  value_(value),
  type_(type),
  bezier_control_in_(QPointF(-1.0, 0.0)),
  bezier_control_out_(QPointF(1.0, 0.0)),
  track_(track)
{
}

NodeKeyframePtr NodeKeyframe::Create(const rational &time, const QVariant &value, const NodeKeyframe::Type &type, const int& track)
{
  return std::make_shared<NodeKeyframe>(time, value, type, track);
}

NodeKeyframePtr NodeKeyframe::copy() const
{
  NodeKeyframePtr copy = std::make_shared<NodeKeyframe>(time_, value_, type_, track_);
  copy->bezier_control_in_ = bezier_control_in_;
  copy->bezier_control_out_ = bezier_control_out_;
  return copy;
}

const rational &NodeKeyframe::time() const
{
  return time_;
}

void NodeKeyframe::set_time(const rational &time)
{
  time_ = time;
  emit TimeChanged(time_);
}

const QVariant &NodeKeyframe::value() const
{
  return value_;
}

void NodeKeyframe::set_value(const QVariant &value)
{
  value_ = value;
  emit ValueChanged(value_);
}

const NodeKeyframe::Type &NodeKeyframe::type() const
{
  return type_;
}

void NodeKeyframe::set_type(const NodeKeyframe::Type &type)
{
  type_ = type;
  emit TypeChanged(type_);
}

const QPointF &NodeKeyframe::bezier_control_in() const
{
  return bezier_control_in_;
}

void NodeKeyframe::set_bezier_control_in(const QPointF &control)
{
  bezier_control_in_ = control;
  emit BezierControlInChanged(bezier_control_in_);
}

const QPointF &NodeKeyframe::bezier_control_out() const
{
  return bezier_control_out_;
}

void NodeKeyframe::set_bezier_control_out(const QPointF &control)
{
  bezier_control_out_ = control;
  emit BezierControlOutChanged(bezier_control_out_);
}

const QPointF &NodeKeyframe::bezier_control(NodeKeyframe::BezierType type) const
{
  if (type == kInHandle) {
    return bezier_control_in();
  } else {
    return bezier_control_out();
  }
}

void NodeKeyframe::set_bezier_control(NodeKeyframe::BezierType type, const QPointF &control)
{
  if (type == kInHandle) {
    set_bezier_control_in(control);
  } else {
    set_bezier_control_out(control);
  }
}

const int &NodeKeyframe::track() const
{
  return track_;
}

NodeKeyframe::BezierType NodeKeyframe::get_opposing_bezier_type(NodeKeyframe::BezierType type)
{
  if (type == kInHandle) {
    return kOutHandle;
  } else {
    return kInHandle;
  }
}

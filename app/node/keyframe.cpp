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

NodeKeyframe::NodeKeyframe() :
  time_(0),
  value_(0),
  type_(kLinear)
{

}

const rational &NodeKeyframe::time()
{
  return time_;
}

void NodeKeyframe::set_time(const rational &time)
{
  time_ = time;
}

const QVariant &NodeKeyframe::value()
{
  return value_;
}

void NodeKeyframe::set_value(const QVariant &value)
{
  value_ = value;
}

const NodeKeyframe::Type &NodeKeyframe::type()
{
  return type_;
}

void NodeKeyframe::set_type(const NodeKeyframe::Type &type)
{
  type_ = type;
}

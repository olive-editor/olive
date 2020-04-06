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

#include "dependency.h"

#include "node.h"

OLIVE_NAMESPACE_ENTER

NodeDependency::NodeDependency() :
  node_(nullptr)
{
}

NodeDependency::NodeDependency(const Node *node, const TimeRange &range) :
  node_(node),
  range_(range)
{
}

NodeDependency::NodeDependency(const Node *node, const rational &in, const rational &out) :
  node_(node),
  range_(in, out)
{
}

const Node *NodeDependency::node() const
{
  return node_;
}

const rational& NodeDependency::in() const
{
  return range_.in();
}

const rational &NodeDependency::out() const
{
  return range_.out();
}

const TimeRange &NodeDependency::range() const
{
  return range_;
}

OLIVE_NAMESPACE_EXIT

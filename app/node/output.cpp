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

#include "output.h"

#include "node/node.h"

NodeOutput::NodeOutput(const QString &id) :
  NodeParam(id),
  linked_input_(nullptr)
{
}

NodeParam::Type NodeOutput::type()
{
  return kOutput;
}

QVariant NodeOutput::get_value(const rational& in, const rational& out)
{
  mutex_.lock();

  QVariant v;

  if (in_ != in || out_ != out || !value_caching_) {
    // Update the value
    value_ = parent()->Run(this, in, out);

    in_ = in;
    out_ = out;
  }

  v = value_;

  mutex_.unlock();

  return v;
}

void NodeOutput::push_value(const QVariant &v, const rational &in, const rational &out)
{
  value_ = v;
  in_ = in;
  out_ = out;
}

NodeInput *NodeOutput::linked_input()
{
  return linked_input_;
}

void NodeOutput::set_linked_input(NodeInput *link)
{
  linked_input_ = link;
}

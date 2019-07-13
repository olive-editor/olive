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

#include "node.h"

Node::Node(QObject *parent) :
  QObject(parent)
{
}

QString Node::Category()
{
  // Return an empty category for any nodes that don't use one
  return QString();
}

QString Node::Description()
{
  // Return an empty string by default
  return QString();
}

void Node::InvalidateCache(const rational &start_range, const rational &end_range)
{
  Q_UNUSED(start_range)
  Q_UNUSED(end_range)

  /*
  ParamList params = Parameters();

  for (int i=0;i<params.size();i++) {
    NodeParam* param = params.at(i);

    if (param->type() == NodeParam::kOutput
        && param->) {

    }
  }
  */
}

Node::ParamList Node::Parameters()
{
  const QObjectList& child_list = children();

  ParamList params;
  params.reserve(child_list.size());

  for (int i=0;i<child_list.size();i++) {
    params[i] = static_cast<NodeParam*>(child_list.at(i));
  }

  return params;
}

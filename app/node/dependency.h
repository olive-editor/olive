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

#ifndef NODEDEPENDENCY_H
#define NODEDEPENDENCY_H

#include <QMetaType>

#include "common/timerange.h"

OLIVE_NAMESPACE_ENTER

class Node;

class NodeDependency {
public:
  NodeDependency();
  NodeDependency(const Node* node, const TimeRange& range);
  NodeDependency(const Node* node, const rational& in, const rational &out);

  const Node* node() const;
  const rational& in() const;
  const rational& out() const;
  const TimeRange& range() const;

private:
  const Node* node_;
  TimeRange range_;
};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeDependency)

#endif // NODEDEPENDENCY_H

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

#include "common/rational.h"
#include "node/output.h"

class NodeDependency {
public:
  NodeDependency();
  NodeDependency(NodeOutput* node, const rational& time);

  NodeOutput* node() const;
  const rational& time() const;

private:
  NodeOutput* node_;
  rational time_;
};

Q_DECLARE_METATYPE(NodeDependency)

#endif // NODEDEPENDENCY_H

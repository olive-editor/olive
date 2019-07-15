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

#ifndef NODE_H
#define NODE_H

#include <QObject>

#include "common/rational.h"
#include "node/input.h"
#include "node/output.h"

class Node : public QObject
{
  Q_OBJECT
public:
  Node(QObject* parent = nullptr);

  virtual QString Name() = 0;
  virtual QString Category();
  virtual QString Description();

  /**
   * @brief Signal all dependent Nodes that anything cached between start_range and end_range is now invalid and
   *        requires re-rendering
   *
   * Override this if your Node subclass keeps a cache, but call this base function at the end of the subclass function.
   * Default behavior is to relay this signal to all connected outputs, which will need to be done as to not break
   * the DAG. Even if the time needs to be transformed somehow (e.g. converting media time to sequence time), you can
   * call this function with transformed time and relay the signal that way.
   */
  virtual void InvalidateCache(const rational& start_range, const rational& end_range);

  /**
   * @brief Return the parameter at a given index
   */
  NodeParam* ParamAt(int index);

  /**
   * @brief Return a list of NodeParams
   */
  QList<NodeParam*> parameters();

  /**
   * @brief Return the current number of parameters
   */
  int ParameterCount();

  /**
   * @brief Return the index of a parameter
   * @return Parameter index or -1 if this parameter is not part of this Node
   */
  int IndexOfParameter(NodeParam* param);

public slots:
  virtual void Process(const rational& time) = 0;
};

#endif // NODE_H

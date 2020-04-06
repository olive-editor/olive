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

#ifndef EDGE_H
#define EDGE_H

#include <memory>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class NodeOutput;
class NodeInput;

/**
 * @brief A connection between two node parameters (a NodeOutput and a NodeInput)
 *
 * To simplify memory management, it's recommended to use NodeEdgePtr instead of raw pointers when working with
 * NodeEdge.
 */
class NodeEdge
{
public:
  /**
   * @brief Create a node edge connecting an output to an input
   */
  NodeEdge(NodeOutput* output, NodeInput* input);

  /**
   * @brief Return the output parameter this edge is connected to
   */
  NodeOutput* output();

  /**
   * @brief Return the input parameter this edge is connected to
   */
  NodeInput* input();

private:
  NodeOutput* output_;
  NodeInput* input_;
};

using NodeEdgePtr = std::shared_ptr<NodeEdge>;

OLIVE_NAMESPACE_EXIT

#endif // EDGE_H

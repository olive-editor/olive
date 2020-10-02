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

#include "edge.h"

#include "input.h"
#include "node.h"
#include "output.h"

OLIVE_NAMESPACE_ENTER

NodeEdge::NodeEdge(NodeOutput *output, NodeInput *input)
{
  output_ = ParamToConnection(output);
  input_ = ParamToConnection(input);
}

NodeOutput *NodeEdge::output() const
{
  return output_.node->GetOutputWithID(output_.id);
}

NodeInput *NodeEdge::input() const
{
  return input_.node->GetInputWithID(input_.id);
}

NodeEdge::Connection NodeEdge::ParamToConnection(NodeParam *param)
{
  return {param->parentNode(), param->id()};
}

OLIVE_NAMESPACE_EXIT

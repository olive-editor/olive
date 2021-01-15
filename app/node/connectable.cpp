/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "connectable.h"

#include "input.h"
#include "node.h"

namespace olive {

void NodeConnectable::ConnectEdge(Node *output, NodeInput *input, int element)
{
  InputConnection conn_to_in = {input, element};

  // Connection exists
  if (std::find(output->output_connections_.begin(), output->output_connections_.end(), conn_to_in) != output->output_connections_.end()) {
    qDebug() << "Ignored connect that already exists";
    return;
  }

  // Ensure a connection isn't getting overwritten
  Q_ASSERT(input->input_connections_.find(element) == input->input_connections_.end());

  // Insert connections in both sides
  output->output_connections_.push_back(conn_to_in);
  input->input_connections_[element] = output;

  // Emit signals
  emit input->InputConnected(output, element);
  emit output->OutputConnected(input, element);
}

void NodeConnectable::DisconnectEdge(Node *output, NodeInput *input, int element)
{
  InputConnection conn_to_in = {input, element};

  // Connection exists
  if (std::find(output->output_connections_.begin(), output->output_connections_.end(), conn_to_in) == output->output_connections_.end()) {
    qDebug() << "Ignored disconnect that doesn't exist";
    return;
  }

  // Assertions to ensure connection exists
  Q_ASSERT(input->input_connections_.at(element) == output);

  // Remove connections from both sides
  output->output_connections_.erase(std::find(output->output_connections_.begin(), output->output_connections_.end(), conn_to_in));
  input->input_connections_.erase(input->input_connections_.find(element));

  // Emit signals
  emit input->InputDisconnected(output, element);
  emit output->OutputDisconnected(input, element);
}

uint qHash(const NodeConnectable::InputConnection &r, uint seed)
{
  return ::qHash(r.input, seed) ^ ::qHash(r.element, seed);
}

}

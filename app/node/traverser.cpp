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

#include "traverser.h"

#include "node.h"

OLIVE_NAMESPACE_ENTER

NodeValueDatabase NodeTraverser::GenerateDatabase(const Node* node, const TimeRange &range)
{
  NodeValueDatabase database;

  // We need to insert tables into the database for each input
  QList<NodeInput*> inputs = node->GetInputsIncludingArrays();

  foreach (NodeInput* input, inputs) {
    if (IsCancelled()) {
      return NodeValueDatabase();
    }

    TimeRange input_time = node->InputTimeAdjustment(input, range);

    NodeValueTable table = ProcessInput(input, input_time);

    database.Insert(input, table);
  }

  // Insert global variables
  NodeValueTable global;
  global.Push(NodeParam::kFloat, range.in().toDouble(), nullptr, QStringLiteral("time_in"));
  global.Push(NodeParam::kFloat, range.out().toDouble(), nullptr, QStringLiteral("time_out"));
  database.Insert(QStringLiteral("global"), global);

  return database;
}

NodeValueTable NodeTraverser::ProcessInput(NodeInput* input, const TimeRange& range)
{
  if (input->is_connected()) {
    // Value will equal something from the connected node, follow it
    return GenerateTable(input->get_connected_node(), range);
  } else if (!input->IsArray()) {
    // Push onto the table the value at this time from the input
    QVariant input_value = input->get_value_at_time(range.in());

    NodeValueTable table;
    table.Push(input->data_type(), input_value, input->parentNode());
    return table;
  }

  return NodeValueTable();
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const TimeRange& range)
{
  if (n->IsTrack()) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return GenerateBlockTable(static_cast<const TrackOutput*>(n), range);
  }

  // FIXME: Cache certain values here if we've already processed them before

  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(n, range);

  // By this point, the node should have all the inputs it needs to render correctly
  NodeValueTable table = n->Value(database);

  ProcessNodeEvent(n, range, database, table);

  return table;
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const rational &in, const rational &out)
{
  return GenerateTable(n, TimeRange(in, out));
}

NodeValueTable NodeTraverser::GenerateBlockTable(const TrackOutput *track, const TimeRange &range)
{
  // By default, just follow the in point
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = GenerateTable(active_block, range);
  }

  return table;
}

StreamPtr NodeTraverser::ResolveStreamFromInput(NodeInput *input)
{
  return input->get_standard_value().value<StreamPtr>();
}

OLIVE_NAMESPACE_EXIT

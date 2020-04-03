#include "traverser.h"

#include "node.h"

NodeTraverser::NodeTraverser()
{

}

NodeValueDatabase NodeTraverser::GenerateDatabase(const Node* node, const TimeRange &range)
{
  NodeValueDatabase database;

  // We need to insert tables into the database for each input
  foreach (NodeParam* param, node->parameters()) {
    if (IsCancelled()) {
      return NodeValueDatabase();
    }

    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);
      TimeRange input_time = node->InputTimeAdjustment(input, range);

      NodeValueTable table = ProcessInput(input, input_time);

      InputProcessingEvent(input, input_time, &table);

      database.Insert(input, table);
    }
  }

  // Insert global variables
  NodeValueTable global;
  global.Push(NodeParam::kFloat, range.in().toDouble(), QStringLiteral("time_in"));
  global.Push(NodeParam::kFloat, range.out().toDouble(), QStringLiteral("time_out"));
  database.Insert(QStringLiteral("global"), global);

  return database;
}

NodeValueTable NodeTraverser::ProcessNode(const NodeDependency& dep)
{
  const Node* node = dep.node();

  if (node->IsTrack()) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return RenderBlock(static_cast<const TrackOutput*>(node), dep.range());
  }

  // FIXME: Cache certain values here if we've already processed them before

  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(node, dep.range());

  // By this point, the node should have all the inputs it needs to render correctly
  NodeValueTable table = node->Value(database);

  ProcessNodeEvent(node, dep.range(), database, &table);

  return table;
}

NodeValueTable NodeTraverser::RenderBlock(const TrackOutput *track, const TimeRange &range)
{
  // By default, don't bother traversing blocks
  return NodeValueTable();
}

NodeValueTable NodeTraverser::ProcessInput(const NodeInput *input, const TimeRange& range)
{
  if (input->IsConnected()) {
    // Value will equal something from the connected node, follow it
    return ProcessNode(NodeDependency(input->get_connected_node(), range));
  } else {
    // Push onto the table the value at this time from the input
    QVariant input_value = input->get_value_at_time(range.in());

    NodeValueTable table;
    table.Push(input->data_type(), input_value);
    return table;
  }
}

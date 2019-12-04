#include "renderworker.h"

#include <QThread>

#include "node/block/block.h"

RenderWorker::RenderWorker(DecoderCache *decoder_cache, QObject *parent) :
  QObject(parent),
  working_(0),
  started_(false),
  decoder_cache_(decoder_cache)
{
}

bool RenderWorker::IsAvailable()
{
  return (working_ == 0);
}

bool RenderWorker::Init()
{
  if (started_) {
    return true;
  }

  if (!(started_ = InitInternal())) {
    Close();
  }

  return started_;
}

void RenderWorker::Close()
{
  CloseInternal();

  started_ = false;
}

void RenderWorker::Render(NodeDependency path)
{
  emit CompletedCache(path, RenderInternal(path));
}

NodeValueTable RenderWorker::RenderAsSibling(NodeDependency dep)
{
  NodeOutput* output = dep.node();
  Node* node = output->parentNode();
  QList<NodeInput*> connected_inputs;
  NodeValueTable value;

  // Set working state
  working_++;

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if (node->IsBlock()
      && (dep.range().in() < static_cast<Block*>(node)->in()
          || dep.range().out() > static_cast<Block*>(node)->out())) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    value = RenderBlock(output, dep.range());
  } else {
    value = ProcessNodeNormally(NodeDependency(output, dep.range()));
  }

  // We're done!

  // End this working state
  working_--;

  return value;
}

DecoderCache *RenderWorker::decoder_cache()
{
  return decoder_cache_;
}

Block *RenderWorker::ValidateBlock(Block *block, const rational& time)
{
  Q_ASSERT(block != nullptr && time >= 0);

  while (block->in() > time) {
    // This Block is too late, find an earlier one
    block = block->previous();
  }

  while (block->out() <= time) {
    // This block is too early, find a later one
    if (block->next() == nullptr) {
      break;
    }

    block = block->next();
  }

  // By this point, we should have the correct Block or nullptr if there's no Block here
  return block;
}

QList<Block *> RenderWorker::ValidateBlockRange(Block *n, const TimeRange &range)
{
  QList<Block*> list;
  Block* block_at_start = ValidateBlock(n, range.in());
  Block* block_at_end = ValidateBlock(n, range.out());

  list.append(block_at_start);

  // If more than one block is active for this range
  if (block_at_start != block_at_end) {

    // Collect all blocks between the start and the end
    do {
      block_at_start = block_at_start->next();
      list.append(block_at_start);
    } while (block_at_start != block_at_end);
  }

  return list;
}

NodeValueTable RenderWorker::RenderInternal(const NodeDependency &path)
{
  return RenderAsSibling(path);
}

StreamPtr RenderWorker::ResolveStreamFromInput(NodeInput *input)
{
  return input->get_value_at_time(0).value<StreamPtr>();
}

DecoderPtr RenderWorker::ResolveDecoderFromInput(NodeInput *input)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!
  StreamPtr stream = ResolveStreamFromInput(input);
  DecoderPtr decoder = decoder_cache()->GetDecoder(stream.get());

  if (decoder == nullptr && stream != nullptr) {
    // Create a new Decoder here
    DecoderPtr decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);
    decoder_cache()->AddDecoder(stream.get(), decoder);
  }

  return decoder;
}

bool RenderWorker::IsStarted()
{
  return started_;
}

NodeValueTable RenderWorker::ProcessNodeNormally(const NodeDependency& dep)
{
  NodeOutput* output = dep.node();
  Node* node = dep.node()->parentNode();

  //qDebug() << "Processing" << node->id();

  // FIXME: Cache certain values here if we've already processed them before

  NodeValueDatabase database;

  // We need to insert tables into the database for each input
  foreach (NodeParam* param, node->parameters()) {
    if (param->type() == NodeParam::kInput) {
      NodeValueTable table;
      NodeInput* input = static_cast<NodeInput*>(param);
      TimeRange input_time = node->InputTimeAdjustment(input, dep.range());

      if (input->IsConnected()) {
        // Value will equal something from the connected node, follow it
        table = ProcessNodeNormally(NodeDependency(input->get_connected_output(),
                                                   input_time));
      } else {
        // Push onto the table the value at this time from the input
        QVariant input_value = input->get_value_at_time(input_time.in());
        table.Push(input->data_type(), input_value);
      }

      database.Insert(input, table);
    }
  }

  // By this point, the node should have all the inputs it needs to render correctly

  // Check if we have a shader for this output
  if (OutputIsAccelerated(output)) {
    // Run code
    return RunNodeAccelerated(output);
  } else {
    // Generate the value as expected
    return node->Value(database);
  }
}

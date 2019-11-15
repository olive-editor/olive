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
  NodeOutput* output = path.node();
  Node* node = output->parent();

  QList<Node*> all_nodes_in_graph = ListNodeAndAllDependencies(node);

  // Lock all Nodes to prevent UI changes during this render
  foreach (Node* dep, all_nodes_in_graph) {
    dep->LockUserInput();
  }

  RenderInternal(path);

  emit CompletedCache(path);

  // Unlock all Nodes so changes can be made again
  foreach (Node* dep, all_nodes_in_graph) {
    dep->UnlockUserInput();
  }
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

void RenderWorker::RenderInternal(const NodeDependency &path)
{
  RenderAsSibling(path);
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
    // Init decoder
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);
    decoder_cache()->AddDecoder(stream.get(), decoder);
  }

  return decoder;
}

QList<Node *> RenderWorker::ListNodeAndAllDependencies(Node *n)
{
  QList<Node*> node_list;

  node_list.append(n);
  node_list.append(n->GetDependencies());

  return node_list;
}

bool RenderWorker::IsStarted()
{
  return started_;
}

QList<NodeInput*> RenderWorker::ProcessNodeInputsForTime(Node *n, const TimeRange &time)
{
  QList<NodeInput*> connected_inputs;

  // Now we need to gather information about this Node's inputs
  foreach (NodeParam* param, n->parameters()) {
    // Check if this parameter is an input and if the Node is dependent on it
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      if (input->dependent()) {
        // If we're here, this input is necessary and we need to acquire the value for this Node
        if (input->IsConnected()) {
          // If it's connected to something, we need to retrieve that output at some point
          connected_inputs.append(input);
        } else {
          // If it isn't connected, it'll have the value we need inside it. We just need to store it for the node.
          input->set_stored_value(input->get_value_at_time(n->InputTimeAdjustment(input, time).in()));
        }

        // Special types like FOOTAGE require extra work from us (to decrease node complexity dealing with decoders)
        if (input->data_type() == NodeParam::kFootage) {
          input->set_stored_value(0);

          DecoderPtr decoder = ResolveDecoderFromInput(input);

          // By this point we should definitely have a decoder, and if we don't something's gone terribly wrong
          if (decoder != nullptr) {
            FramePtr frame = decoder->Retrieve(time.in(), time.out() - time.in());

            if (frame != nullptr) {
              QVariant value = FrameToValue(frame);

              input->set_stored_value(value);
            }
          }
        }
      }
    }
  }

  return connected_inputs;
}

QVariant RenderWorker::ProcessNodeNormally(const NodeDependency& dep)
{
  NodeOutput* output = dep.node();
  Node* node = dep.node()->parent();

  //qDebug() << "Processing" << node->id();

  // Check if the output already has a value for this time
  if (output->has_cached_value(dep.range())) {
    // If so, we don't need to do anything, we can just send this value and exit here
    return output->get_cached_value(dep.range());
  }

  // We need to run the Node's code to get the correct value for this time

  QList<NodeInput*> connected_inputs = ProcessNodeInputsForTime(node, dep.range());

  // For each connected input, we need to acquire the value from another node
  while (!connected_inputs.isEmpty()) {

    // Remove any inputs from the list that we have valid cached values for already
    for (int i=0;i<connected_inputs.size();i++) {
      NodeInput* input = connected_inputs.at(i);
      NodeOutput* connected_output = input->get_connected_output();
      TimeRange input_time = node->InputTimeAdjustment(input, dep.range());

      if (connected_output->has_cached_value(input_time)) {
        // This output already has this value, no need to process it again
        input->set_stored_value(connected_output->get_cached_value(input_time));
        connected_inputs.removeAt(i);
        i--;
      }
    }

    // For every connected input except the first, we'll request another Node to do it
    int input_for_this_thread = -1;

    for (int i=0;i<connected_inputs.size();i++) {
      NodeInput* input = connected_inputs.at(i);

      // If this node is locked, we assume it's already being processed. Otherwise we need to request a sibling
      if (!input->get_connected_node()->IsProcessingLocked()) {
        if (input_for_this_thread == -1) {
          // Store this later since we can process it on this thread as we wait for other threads
          input_for_this_thread = i;
        } else {
          TimeRange input_time = node->InputTimeAdjustment(input, dep.range());

          emit RequestSibling(NodeDependency(input->get_connected_output(),
                                             input_time));
        }
      }
    }

    if (input_for_this_thread > -1) {
      // In the mean time, this thread can go off to do the first parameter
      NodeInput* input = connected_inputs.at(input_for_this_thread);
      TimeRange input_range = node->InputTimeAdjustment(input, dep.range());
      RenderAsSibling(NodeDependency(input->get_connected_output(),
                                     input_range));
      input->set_stored_value(input->get_connected_output()->get_cached_value(input_range));
      connected_inputs.removeAt(input_for_this_thread);
    } else {
      // Nothing for this thread to do. We'll wait 0.5 sec and check again for other nodes
      // FIXME: It would be nicer if this thread could do other nodes during this time
      QThread::msleep(500);
    }
  }

  // By this point, the node should have all the inputs it needs to render correctly

  // Check if we have a shader for this output
  if (OutputIsAccelerated(output)) {
    // Run code
    return RunNodeAccelerated(output);
  } else {
    // Generate the value as expected
    return node->Value(output);
  }
}

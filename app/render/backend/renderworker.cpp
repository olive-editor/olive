#include "renderworker.h"

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

  // Unlock all Nodes so changes can be made again
  foreach (Node* dep, all_nodes_in_graph) {
    dep->UnlockUserInput();
  }
}

DecoderCache *RenderWorker::decoder_cache()
{
  return decoder_cache_;
}

Node *RenderWorker::ValidateBlock(Node *n, const rational& time)
{
  if (n->IsBlock()) {
    Block* block = static_cast<Block*>(n);

    while (block != nullptr && block->in() > time) {
      // This Block is too late, find an earlier one
      block = block->previous();
    }

    while (block != nullptr && block->out() <= time) {
      // This block is too early, find a later one
      block = block->next();
    }

    // By this point, we should have the correct Block or nullptr if there's no Block here
    return block;
  }

  return n;
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

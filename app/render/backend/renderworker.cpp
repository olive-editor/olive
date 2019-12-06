#include "renderworker.h"

#include <QThread>

#include "node/block/block.h"

RenderWorker::RenderWorker(QObject *parent) :
  QObject(parent),
  working_(0),
  started_(false)
{
}

bool RenderWorker::IsAvailable()
{
  return !working_;
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

  decoder_cache_.Clear();

  started_ = false;
}

void RenderWorker::Render(NodeDependency path)
{
  emit CompletedCache(path, RenderInternal(path));
}

NodeValueTable RenderWorker::RenderAsSibling(NodeDependency dep)
{
  Node* node = dep.node();
  QList<NodeInput*> connected_inputs;
  NodeValueTable value;

  //qDebug() << "Processing" << node->id();

  // Set working state
  working_++;

  // Firstly we check if this node is a "Block", if it is that means it's part of a linked list of mutually exclusive
  // nodes based on time and we might need to locate which Block to attach to
  if (node->IsTrack()) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    value = RenderBlock(static_cast<TrackOutput*>(node), dep.range());
  } else {
    value = ProcessNodeNormally(NodeDependency(node, dep.range()));
  }

  // We're done!

  // End this working state
  working_--;

  return value;
}

NodeValueTable RenderWorker::RenderInternal(const NodeDependency &path)
{
  return RenderAsSibling(path);
}

bool RenderWorker::OutputIsAccelerated(Node *output)
{
  Q_UNUSED(output)
  return false;
}

void RenderWorker::RunNodeAccelerated(Node *node, const NodeValueDatabase *input_params, NodeValueTable* output_params)
{
  Q_UNUSED(node)
  Q_UNUSED(input_params)
  Q_UNUSED(output_params)
}

StreamPtr RenderWorker::ResolveStreamFromInput(NodeInput *input)
{
  return input->get_value_at_time(0).value<StreamPtr>();
}

DecoderPtr RenderWorker::ResolveDecoderFromInput(StreamPtr stream)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!
  DecoderPtr decoder = decoder_cache_.Get(stream.get());

  if (decoder == nullptr && stream != nullptr) {
    // Create a new Decoder here
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);
    decoder_cache_.Add(stream.get(), decoder);
  }

  return decoder;
}

bool RenderWorker::IsStarted()
{
  return started_;
}

NodeValueTable RenderWorker::ProcessNodeNormally(const NodeDependency& dep)
{
  Node* node = dep.node();

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
        table = ProcessNodeNormally(NodeDependency(input->get_connected_node(),
                                                   input_time));
      } else {
        // Push onto the table the value at this time from the input
        QVariant input_value = input->get_value_at_time(input_time.in());
        table.Push(input->data_type(), input_value);
      }

      // Exception for Footage types where we actually retrieve some Footage data from a decoder
      if (input->data_type() == NodeParam::kFootage) {
        StreamPtr stream = ResolveStreamFromInput(input);

        if (stream) {
          DecoderPtr decoder = ResolveDecoderFromInput(stream);

          if (decoder) {
            FramePtr frame = RetrieveFromDecoder(decoder, input_time);

            if (frame) {
              FrameToValue(stream, frame, &table);
            }
          }
        }
      }

      database.Insert(input, table);
    }
  }

  // By this point, the node should have all the inputs it needs to render correctly

  NodeValueTable table = node->Value(database);

  // Check if we have a shader for this output
  RunNodeAccelerated(node, &database, &table);

  return table;
}

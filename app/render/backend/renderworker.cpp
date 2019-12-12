#include "renderworker.h"

#include <QThread>

#include "node/block/block.h"

RenderWorker::RenderWorker(QObject *parent) :
  QObject(parent),
  started_(false)
{
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

NodeValueTable RenderWorker::RenderInternal(const NodeDependency &path)
{
  return ProcessNode(path);
}

bool RenderWorker::OutputIsAccelerated(Node *output)
{
  Q_UNUSED(output)
  return false;
}

void RenderWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase *input_params, NodeValueTable* output_params)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
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

NodeValueTable RenderWorker::ProcessNode(const NodeDependency& dep)
{
  const Node* node = dep.node();

  if (node->IsTrack()) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return RenderBlock(static_cast<const TrackOutput*>(node), dep.range());
  }

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
        table = ProcessNode(NodeDependency(input->get_connected_node(),
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
  RunNodeAccelerated(node, dep.range(), &database, &table);

  return table;
}

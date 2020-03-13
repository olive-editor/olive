#include "renderworker.h"

#include <QThread>

#include "node/block/block.h"

RenderWorker::RenderWorker(DecoderCache *decoder_cache, QObject *parent) :
  QObject(parent),
  started_(false),
  decoder_cache_(decoder_cache)
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

  started_ = false;
}

void RenderWorker::Render(NodeDependency path, qint64 job_time)
{
  path_ = path;

  emit CompletedCache(path, RenderInternal(path, job_time), job_time);
}

NodeValueTable RenderWorker::RenderInternal(const NodeDependency &path, const qint64 &job_time)
{
  Q_UNUSED(job_time)

  return ProcessNode(path);
}

void RenderWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params)
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
  QMutexLocker locker(decoder_cache_->lock());

  // Access a map of Node inputs and decoder instances and retrieve a frame!

  DecoderPtr decoder = decoder_cache_->Get(stream.get());

  if (!decoder && stream) {
    // Create a new Decoder here
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);

    if (decoder->Open()) {
      decoder_cache_->Add(stream.get(), decoder);
    } else {
      decoder = nullptr;
      qWarning() << "Failed to open decoder for" << stream->footage()->filename() << "::" << stream->index();
    }
  }

  return decoder;
}

bool RenderWorker::IsStarted()
{
  return started_;
}

void RenderWorker::ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational& stream_time)
{
  emit FootageUnavailable(stream, state, path_.range(), stream_time);
}

void RenderWorker::InputProcessingEvent(NodeInput* input, const TimeRange& input_time, NodeValueTable *table)
{
  // Exception for Footage types where we actually retrieve some Footage data from a decoder
  if (input->data_type() == NodeParam::kFootage) {
    StreamPtr stream = ResolveStreamFromInput(input);

    if (stream) {
      DecoderPtr decoder = ResolveDecoderFromInput(stream);

      if (decoder) {

        Decoder::RetrieveState state = decoder->GetRetrieveState(input_time.out());

        if (state == Decoder::kReady) {
          FrameToValue(decoder, stream, input_time, table);
        } else {
          ReportUnavailableFootage(stream, state, input_time.out());
        }
      }
    }
  }
}

void RenderWorker::ProcessNodeEvent(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params)
{
  // Check if we have a shader for this output
  RunNodeAccelerated(node, range, input_params, output_params);
}

const NodeDependency &RenderWorker::CurrentPath() const
{
  return path_;
}

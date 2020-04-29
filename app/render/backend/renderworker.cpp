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

#include "renderworker.h"

#include <QThread>

#include "node/block/block.h"

OLIVE_NAMESPACE_ENTER

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

void RenderWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, NodeValueDatabase &input_params, NodeValueTable& output_params)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(input_params)
  Q_UNUSED(output_params)
}

StreamPtr RenderWorker::ResolveStreamFromInput(NodeInput *input)
{
  return input->get_standard_value().value<StreamPtr>();
}

DecoderPtr RenderWorker::ResolveDecoderFromInput(StreamPtr stream)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!

  DecoderPtr decoder = decoder_cache_.Get(stream.get());

  if (!decoder && stream) {
    // Create a new Decoder here
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);

    if (decoder->Open()) {
      decoder_cache_.Add(stream.get(), decoder);
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

void RenderWorker::ProcessNodeEvent(const Node *node, const TimeRange &range, NodeValueDatabase &input_params, NodeValueTable &output_params)
{
  // Check if we have a shader for this output
  RunNodeAccelerated(node, range, input_params, output_params);
}

const NodeDependency &RenderWorker::CurrentPath() const
{
  return path_;
}

OLIVE_NAMESPACE_EXIT

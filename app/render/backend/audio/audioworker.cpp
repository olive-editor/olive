#include "audioworker.h"

AudioWorker::AudioWorker(QObject *parent) :
  AudioRenderWorker(parent)
{
}

void AudioWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  Q_UNUSED(stream)

  table->Push(NodeParam::kSamples, frame->ToByteArray());
}

void AudioWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase *input_params, NodeValueTable *output_params)
{
  // Check if node processes samples
  if (!node->ProcessesSamplesFrom()) {
    return;
  }

  // Try to find the sample buffer in the table
  QVariant samples_var = (*input_params)[node->ProcessesSamplesFrom()].Get(NodeParam::kSamples);

  // If there isn't one, there's nothing to do
  if (samples_var.isNull()) {
    return;
  }

  QByteArray input_buffer = samples_var.toByteArray();
  QByteArray output_buffer(input_buffer.size(), 0);

  int sample_count = input_buffer.size() / audio_params().bytes_per_sample_per_channel();

  // FIXME: Hardcoded float sample format
  for (int i=0;i<sample_count;i++) {
    node->ProcessSamples(input_params,
                         audio_params(),
                         reinterpret_cast<const float*>(input_buffer.constData()),
                         reinterpret_cast<float*>(output_buffer.data()),
                         i);
  }

  output_params->Push(NodeParam::kSamples, output_buffer);
}

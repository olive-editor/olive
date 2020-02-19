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

void AudioWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params_in, NodeValueTable *output_params)
{
  // Check if node processes samples
  if (!node->ProcessesSamplesFrom()) {
    return;
  }

  // Copy database so we can make some temporary modifications to it
  NodeValueDatabase input_params = input_params_in;

  // Try to find the sample buffer in the table
  QVariant samples_var = input_params[node->ProcessesSamplesFrom()].Get(NodeParam::kSamples);

  // If there isn't one, there's nothing to do
  if (samples_var.isNull()) {
    return;
  }

  QByteArray input_buffer = samples_var.toByteArray();
  QByteArray output_buffer(input_buffer.size(), 0);

  int sample_count = input_buffer.size() / audio_params().bytes_per_sample_per_channel();

  // FIXME: Hardcoded float sample format
  for (int i=0;i<sample_count;i++) {
    // Calculate the exact rational time at this sample
    int sample_out_of_channel = i / audio_params().channel_count();
    double sample_to_second = static_cast<double>(sample_out_of_channel) / static_cast<double>(audio_params().sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    foreach (NodeParam* param, node->parameters()) {
      if (param->type() == NodeParam::kInput
          && param != node->ProcessesSamplesFrom()) {
        NodeInput* input = static_cast<NodeInput*>(param);

        // If the input isn't keyframing, we don't need to update it unless it's connected, in which case it may change
        if (input->IsConnected() || input->is_keyframing()) {
          input_params.Insert(input, ProcessInput(input, TimeRange(this_sample_time, this_sample_time)));
        }
      }
    }

    node->ProcessSamples(&input_params,
                         audio_params(),
                         reinterpret_cast<const float*>(input_buffer.constData()),
                         reinterpret_cast<float*>(output_buffer.data()),
                         i);
  }

  output_params->Push(NodeParam::kSamples, output_buffer);
}

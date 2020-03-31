#include "audioworker.h"

AudioWorker::AudioWorker(DecoderCache* decoder_cache, QHash<Node *, Node *> *copy_map, QObject *parent) :
  AudioRenderWorker(decoder_cache, copy_map, parent)
{
}

void AudioWorker::FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table)
{
  if (stream->type() != Stream::kAudio) {
    return;
  }

  if (decoder->HasConformedVersion(audio_params())) {
    SampleBufferPtr frame = decoder->RetrieveAudio(range.in(), range.out() - range.in(), audio_params());

    if (frame) {
      table->Push(NodeParam::kSamples, QVariant::fromValue(frame));
    }
  } else {
    emit ConformUnavailable(decoder->stream(), CurrentPath().range(), range.out(), audio_params());
  }
}

void AudioWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params_in, NodeValueTable *output_params)
{
  // Check if node processes samples
  if (!(node->GetCapabilities(input_params_in) & Node::kSampleProcessor)) {
    return;
  }

  // Copy database so we can make some temporary modifications to it
  NodeValueDatabase input_params = input_params_in;
  NodeInput* sample_input = node->ProcessesSamplesFrom(input_params);

  // Try to find the sample buffer in the table
  QVariant samples_var = input_params[sample_input].Get(NodeParam::kSamples);

  // If there isn't one, there's nothing to do
  if (samples_var.isNull()) {
    return;
  }

  SampleBufferPtr input_buffer = samples_var.value<SampleBufferPtr>();

  if (!input_buffer) {
    return;
  }

  SampleBufferPtr output_buffer = SampleBuffer::CreateAllocated(input_buffer->audio_params(), input_buffer->sample_count_per_channel());

  int sample_count = input_buffer->sample_count_per_channel();

  // FIXME: Hardcoded float sample format
  for (int i=0;i<sample_count;i++) {
    // Calculate the exact rational time at this sample
    int sample_out_of_channel = i / audio_params().channel_count();
    double sample_to_second = static_cast<double>(sample_out_of_channel) / static_cast<double>(audio_params().sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    foreach (NodeParam* param, node->parameters()) {
      if (param->type() == NodeParam::kInput
          && param != sample_input) {
        NodeInput* input = static_cast<NodeInput*>(param);

        // If the input isn't keyframing, we don't need to update it unless it's connected, in which case it may change
        if (input->IsConnected() || input->is_keyframing()) {
          input_params.Insert(input, ProcessInput(input, TimeRange(this_sample_time, this_sample_time)));
        }
      }
    }

    node->ProcessSamples(input_params,
                         audio_params(),
                         input_buffer,
                         output_buffer,
                         i);
  }

  output_params->Push(NodeParam::kSamples, QVariant::fromValue(output_buffer));
}

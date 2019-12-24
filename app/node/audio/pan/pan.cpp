#include "pan.h"

PanNode::PanNode()
{
  samples_input_ = new NodeInput("samples_in");
  samples_input_->set_data_type(NodeParam::kSamples);
  AddInput(samples_input_);

  panning_input_ = new NodeInput("panning_in");
  panning_input_->set_data_type(NodeParam::kFloat);
  panning_input_->set_minimum(-1);
  panning_input_->set_maximum(1);
  panning_input_->set_value_at_time(0, 0);
  AddInput(panning_input_);
}

Node *PanNode::copy() const
{
  return new PanNode();
}

QString PanNode::Name() const
{
  return tr("Pan");
}

QString PanNode::id() const
{
  return "org.olivevideoeditor.Olive.pan";
}

QString PanNode::Category() const
{
  return tr("Audio");
}

QString PanNode::Description() const
{
  return tr("Adjust the stereo panning of an audio source.");
}

bool PanNode::ProcessesSamples() const
{
  return true;
}

void PanNode::ProcessSamples(const NodeValueDatabase &values, const AudioRenderingParams &params, const float *input, float *output, int index) const
{
  if (params.channel_count() != 2) {
    // This node currently only works for stereo audio
    return;
  }

  float pan_val = values[panning_input_].Get(NodeParam::kFloat).toFloat();

  if (index%2 == 0) {
    // Sample is left channel
    if (pan_val > 0) {
      output[index] = input[index] * (1.0F - pan_val);
    }
  } else {
    // Sample is right channel
    if (pan_val < 0) {
      output[index] = input[index] * (1.0F - qAbs(pan_val));
    }
  }
}

#include "pan.h"

PanNode::PanNode()
{
  samples_input_ = new NodeInput("samples_in", NodeParam::kSamples);
  AddInput(samples_input_);

  panning_input_ = new NodeInput("panning_in", NodeParam::kFloat);
  panning_input_->set_property("min", -1.0);
  panning_input_->set_property("max", 1.0);
  panning_input_->set_property("view", "percent");
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

NodeInput *PanNode::ProcessesSamplesFrom() const
{
  return samples_input_;
}

void PanNode::ProcessSamples(const NodeValueDatabase *values, const AudioRenderingParams &params, const float *input, float *output, int index) const
{
  if (params.channel_count() != 2) {
    // This node currently only works for stereo audio
    return;
  }

  float pan_val = (*values)[panning_input_].Get(NodeParam::kFloat).toFloat();

  output[index] = input[index];

  if (index%2 == 0) {
    // Sample is left channel
    if (pan_val > 0) {
      output[index] *= (1.0F - pan_val);
    }
  } else {
    // Sample is right channel
    if (pan_val < 0) {
      output[index] *= (1.0F - qAbs(pan_val));
    }
  }
}

void PanNode::Retranslate()
{
  samples_input_->set_name(tr("Samples"));
  panning_input_->set_name(tr("Pan"));
}

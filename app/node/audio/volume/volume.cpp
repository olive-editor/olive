#include "volume.h"

VolumeNode::VolumeNode()
{
  samples_input_ = new NodeInput("samples_in", NodeParam::kSamples);
  AddInput(samples_input_);

  volume_input_ = new NodeInput("volume_in", NodeParam::kFloat, 1);
  volume_input_->set_property("min", 0.0);
  volume_input_->set_property("view", "db");
  AddInput(volume_input_);
}

Node *VolumeNode::copy() const
{
  return new VolumeNode();
}

QString VolumeNode::Name() const
{
  return tr("Volume");
}

QString VolumeNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.volume");
}

QString VolumeNode::Category() const
{
  return tr("Audio");
}

QString VolumeNode::Description() const
{
  return tr("Adjusts the volume of an audio source.");
}

Node::Capabilities VolumeNode::GetCapabilities(const NodeValueDatabase &) const
{
  return kSampleProcessor;
}

NodeInput *VolumeNode::ProcessesSamplesFrom() const
{
  return samples_input_;
}

void VolumeNode::ProcessSamples(const NodeValueDatabase *values, const AudioRenderingParams& params, const float* input, float* output, int index) const
{
  float volume_val = (*values)[volume_input_].Get(NodeParam::kFloat).toFloat();

  output[index] = input[index] * volume_val;
}

void VolumeNode::Retranslate()
{
  samples_input_->set_name(tr("Samples"));
  volume_input_->set_name(tr("Volume"));
}

NodeInput *VolumeNode::samples_input() const
{
  return samples_input_;
}

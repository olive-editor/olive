#include "audio.h"

AudioInput::AudioInput()
{
  samples_output_ = new NodeOutput("samples_out");
  AddParameter(samples_output_);
}

Node *AudioInput::copy() const
{
  return new AudioInput();
}

QString AudioInput::Name() const
{
  return tr("Audio Input");
}

QString AudioInput::id() const
{
  return "org.olivevideoeditor.Olive.audioinput";
}

QString AudioInput::Category() const
{
  return tr("Input");
}

QString AudioInput::Description() const
{
  return tr("Import an audio footage stream.");
}

NodeOutput *AudioInput::samples_output()
{
  return samples_output_;
}

NodeValueTable AudioInput::Value(const NodeValueDatabase &value) const
{
  return value[footage_input_];
}

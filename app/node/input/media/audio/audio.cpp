#include "audio.h"

AudioInput::AudioInput()
{
  samples_output_ = new NodeOutput("samples_out");
  AddParameter(samples_output_);
}

Node *AudioInput::copy()
{
  return new AudioInput();
}

QString AudioInput::Name()
{
  return tr("Audio Input");
}

QString AudioInput::id()
{
  return "org.olivevideoeditor.Olive.audioinput";
}

QString AudioInput::Category()
{
  return tr("Input");
}

QString AudioInput::Description()
{
  return tr("Import an audio footage stream.");
}

NodeOutput *AudioInput::samples_output()
{
  return samples_output_;
}

QVariant AudioInput::Value(NodeOutput *output)
{
  if (output == samples_output_) {
    // Simple passthrough from footage input
    return footage_input_->value();
  }

  return MediaInput::Value(output);
}

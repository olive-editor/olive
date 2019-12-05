#include "audio.h"

AudioInput::AudioInput()
{
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

NodeValueTable AudioInput::Value(const NodeValueDatabase &value) const
{
  return value[footage_input_];
}

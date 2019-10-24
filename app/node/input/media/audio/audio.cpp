#include "audio.h"

AudioInput::AudioInput()
{

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

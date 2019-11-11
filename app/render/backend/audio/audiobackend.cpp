#include "audiobackend.h"

AudioBackend::AudioBackend(QObject *parent) :
  AudioRenderBackend(parent)
{
}

bool AudioBackend::InitInternal()
{
  // This backend doesn't init anything yet
  return true;
}

void AudioBackend::CloseInternal()
{
  // This backend doesn't init anything yet
}

bool AudioBackend::CompileInternal()
{
  // This backend doesn't compile anything yet
  return true;
}

void AudioBackend::DecompileInternal()
{
  // This backend doesn't compile anything yet
}

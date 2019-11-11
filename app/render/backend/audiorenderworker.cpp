#include "audiorenderworker.h"

AudioRenderWorker::AudioRenderWorker(DecoderCache *decoder_cache, QObject *parent) :
  RenderWorker(decoder_cache, parent)
{
}

void AudioRenderWorker::RenderAsSibling(NodeDependency dep)
{

}

bool AudioRenderWorker::InitInternal()
{
  // Nothing to init yet
  return true;
}

void AudioRenderWorker::CloseInternal()
{
  // Nothing to init yet
}

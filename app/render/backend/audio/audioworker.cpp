#include "audioworker.h"

AudioWorker::AudioWorker(DecoderCache *decoder_cache, QObject *parent) :
  AudioRenderWorker(decoder_cache, parent)
{
}

void AudioWorker::FrameToValue(FramePtr frame, NodeValueTable *table)
{
  table->Push(NodeParam::kSamples, frame->ToByteArray());
}

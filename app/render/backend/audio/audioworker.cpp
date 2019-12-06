#include "audioworker.h"

AudioWorker::AudioWorker(DecoderCache *decoder_cache, QObject *parent) :
  AudioRenderWorker(decoder_cache, parent)
{
}

void AudioWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  Q_UNUSED(stream)
  table->Push(NodeParam::kSamples, frame->ToByteArray());
}

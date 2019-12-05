#include "audioworker.h"

AudioWorker::AudioWorker(DecoderCache *decoder_cache, QObject *parent) :
  AudioRenderWorker(decoder_cache, parent)
{
}

QVariant AudioWorker::FrameToValue(FramePtr frame)
{
  return frame->ToByteArray();
}

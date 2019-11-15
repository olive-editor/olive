#include "audioworker.h"

AudioWorker::AudioWorker(DecoderCache *decoder_cache, QObject *parent) :
  AudioRenderWorker(decoder_cache, parent)
{
}

QVariant AudioWorker::FrameToValue(FramePtr frame)
{
  return frame->ToByteArray();
}

bool AudioWorker::OutputIsAccelerated(NodeOutput *output)
{
  Q_UNUSED(output)
  return false;
}

QVariant AudioWorker::RunNodeAccelerated(NodeOutput *output)
{
  Q_UNUSED(output)
  return QVariant();
}

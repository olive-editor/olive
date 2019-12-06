#include "audioworker.h"

AudioWorker::AudioWorker(QObject *parent) :
  AudioRenderWorker(parent)
{
}

void AudioWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  Q_UNUSED(stream)

  table->Push(NodeParam::kSamples, frame->ToByteArray());
}

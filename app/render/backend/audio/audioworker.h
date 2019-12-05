#ifndef AUDIOWORKER_H
#define AUDIOWORKER_H

#include "../audiorenderworker.h"

class AudioWorker : public AudioRenderWorker
{
public:
  AudioWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

protected:
  virtual void FrameToValue(FramePtr frame, NodeValueTable* table) override;

private:

};

#endif // AUDIOWORKER_H

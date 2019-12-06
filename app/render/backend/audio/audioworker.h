#ifndef AUDIOWORKER_H
#define AUDIOWORKER_H

#include "../audiorenderworker.h"

class AudioWorker : public AudioRenderWorker
{
public:
  AudioWorker(QObject* parent = nullptr);

protected:
  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) override;

private:

};

#endif // AUDIOWORKER_H

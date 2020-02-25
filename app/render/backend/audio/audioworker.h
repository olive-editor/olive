#ifndef AUDIOWORKER_H
#define AUDIOWORKER_H

#include "../audiorenderworker.h"

class AudioWorker : public AudioRenderWorker
{
public:
  AudioWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

protected:
  virtual void FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange& range, const NodeValueDatabase& input_params, NodeValueTable* output_params) override;

private:

};

#endif // AUDIOWORKER_H

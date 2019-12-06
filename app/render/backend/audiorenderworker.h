#ifndef AUDIORENDERWORKER_H
#define AUDIORENDERWORKER_H

#include "renderworker.h"

class AudioRenderWorker : public RenderWorker
{
  Q_OBJECT
public:
  AudioRenderWorker(QObject* parent = nullptr);

  void SetParameters(const AudioRenderingParams& audio_params);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) override;

  virtual NodeValueTable RenderBlock(TrackOutput *track, const TimeRange& range) override;

private:
  AudioRenderingParams audio_params_;

};

#endif // AUDIORENDERWORKER_H

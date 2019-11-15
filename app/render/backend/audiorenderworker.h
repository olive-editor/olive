#ifndef AUDIORENDERWORKER_H
#define AUDIORENDERWORKER_H

#include "renderworker.h"

class AudioRenderWorker : public RenderWorker
{
  Q_OBJECT
public:
  AudioRenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

  void SetParameters(const AudioRenderingParams& audio_params);

public slots:
  virtual void RenderAsSibling(NodeDependency dep) override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  QVariant RenderBlock(NodeOutput *output, const TimeRange& range);

private:
  AudioRenderingParams audio_params_;

};

#endif // AUDIORENDERWORKER_H

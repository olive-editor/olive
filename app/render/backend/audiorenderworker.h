#ifndef AUDIORENDERWORKER_H
#define AUDIORENDERWORKER_H

#include "renderworker.h"

class AudioRenderWorker : public RenderWorker
{
  Q_OBJECT
public:
  AudioRenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

public slots:
  virtual void RenderAsSibling(NodeDependency dep) override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;



};

#endif // AUDIORENDERWORKER_H

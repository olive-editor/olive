#ifndef AUDIORENDERBACKEND_H
#define AUDIORENDERBACKEND_H

#include "renderbackend.h"

class AudioRenderBackend : public RenderBackend
{
  Q_OBJECT
public:
  AudioRenderBackend();

public slots:
  virtual void InvalidateCache(const rational &start_range, const rational &end_range);
};

#endif // AUDIORENDERBACKEND_H

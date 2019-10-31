#include "audiorenderbackend.h"

AudioRenderBackend::AudioRenderBackend()
{

}

void AudioRenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  Q_UNUSED(start_range)
  Q_UNUSED(end_range)
}

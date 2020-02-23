#ifndef DECODERCACHE_H
#define DECODERCACHE_H

#include "codec/decoder.h"
#include "project/item/footage/stream.h"
#include "rendercache.h"

class DecoderCache : public RenderCache<Stream*, DecoderPtr>
{
public:
  DecoderCache() = default;

  QMutex* lock() {return &lock_;}

private:
  QMutex lock_;

};

#endif // DECODERCACHE_H

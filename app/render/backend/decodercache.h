#ifndef DECODERCACHE_H
#define DECODERCACHE_H

#include "decoder/decoder.h"
#include "project/item/footage/stream.h"
#include "rendercache.h"

using DecoderCache = RenderCache<Stream*, DecoderPtr>;

#endif // DECODERCACHE_H

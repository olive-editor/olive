#ifndef COLORPROCESSORCACHE_H
#define COLORPROCESSORCACHE_H

#include "project/item/footage/stream.h"
#include "render/colorprocessor.h"
#include "rendercache.h"

using ColorProcessorCache = RenderCache<Stream*, ColorProcessorPtr>;

#endif // COLORPROCESSORCACHE_H

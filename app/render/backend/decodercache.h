#ifndef DECODERCACHE_H
#define DECODERCACHE_H

#include "decoder/decoder.h"
#include "project/item/footage/stream.h"

/**
 * @brief Thread-safe cache of decoders
 */
class DecoderCache
{
public:
  DecoderCache();

  void Clear();

  void AddDecoder(Stream* stream, DecoderPtr shader);

  DecoderPtr GetDecoder(Stream* stream);

private:
  QMap<Stream*, DecoderPtr> decoders_;

};

#endif // DECODERCACHE_H

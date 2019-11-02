#include "decodercache.h"

DecoderCache::DecoderCache()
{

}

void DecoderCache::Clear()
{
  decoders_.clear();
}

void DecoderCache::AddDecoder(Stream *stream, DecoderPtr shader)
{
  decoders_.insert(stream, shader);
}

DecoderPtr DecoderCache::GetDecoder(Stream *stream)
{
  return decoders_.value(stream);
}

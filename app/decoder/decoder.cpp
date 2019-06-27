#include "decoder.h"

Decoder::Decoder() :
  open_(false)
{
}

Decoder::Decoder(Stream *fs) :
  open_(false),
  stream_(fs)
{
}

Decoder::~Decoder()
{
}

Stream *Decoder::stream()
{
  return stream_;
}

void Decoder::set_stream(Stream *fs)
{
  Close();

  stream_ = fs;
}

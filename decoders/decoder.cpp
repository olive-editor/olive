#include "decoder.h"

Decoder::Decoder(QObject *parent) :
  QObject(parent),
  open_(false)
{
}

Decoder::Decoder(FootageStream *fs) :
  open_(false),
  stream_(fs)
{
}

Decoder::~Decoder()
{
}

FootageStream *Decoder::stream()
{
  return stream_;
}

void Decoder::set_stream(FootageStream *fs)
{
  Close();

  stream_ = fs;
}

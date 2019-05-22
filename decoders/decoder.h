#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>

#include "global/rational.h"

class Decoder
{
public:
  Decoder();
  virtual ~Decoder();

  virtual bool Open() = 0;
  virtual void Request(const rational& timecode) = 0;
  virtual void Retrieve(uint8_t** buffer, int* linesize) = 0;
  virtual void Close() = 0;

  // For video decoding
  virtual int width() = 0;
  virtual int height() = 0;

protected:
  bool open_;

};

#endif // DECODER_H

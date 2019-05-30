#ifndef MEMORYBUFFER_H
#define MEMORYBUFFER_H

#include <QVector>

#include "pixelformats.h"

class MemoryBuffer
{
public:
  MemoryBuffer();

  void Create(int width, int height, const olive::PixelFormat& format);

  const int& width() const;
  const int& height() const;
  const olive::PixelFormat& format() const;
  uint8_t* data();
  const uint8_t* const_data() const;

private:
  QVector<uint8_t> data_;
  int width_;
  int height_;
  olive::PixelFormat format_;
};

#endif // MEMORYBUFFER_H

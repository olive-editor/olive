#include "memorybuffer.h"

#include <QDebug>

#include "decoders/pixelformatconverter.h"

MemoryBuffer::MemoryBuffer()
{
}

void MemoryBuffer::Create(int width, int height, const olive::PixelFormat &format)
{
  width_ = width;
  height_ = height;
  format_ = format;

  data_.resize(olive::pix_fmt_conv->GetBufferSize(format, width, height));
}

const int &MemoryBuffer::width() const
{
  return width_;
}

const int &MemoryBuffer::height() const
{
  return height_;
}

const olive::PixelFormat &MemoryBuffer::format() const
{
  return format_;
}

uint8_t *MemoryBuffer::data()
{
  return data_.data();
}

const uint8_t *MemoryBuffer::const_data() const
{
  return data_.constData();
}

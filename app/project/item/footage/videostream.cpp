#include "videostream.h"

VideoStream::VideoStream(Footage *footage,
                         const int &index,
                         const int &width,
                         const int &height,
                         const rational &timebase) :
  footage_(footage),
  index_(index),
  width_(width),
  height_(height),
  timebase_(timebase)
{
}

Footage *VideoStream::footage()
{
  return footage_;
}

void VideoStream::set_footage(Footage *f)
{
  footage_ = f;
}

const int &VideoStream::index()
{
  return index_;
}

void VideoStream::set_index(const int &index)
{
  index_ = index;
}

const int &VideoStream::width()
{
  return width_;
}

void VideoStream::set_width(const int &width)
{
  width_ = width;
}

const int &VideoStream::height()
{
  return height_;
}

void VideoStream::set_height(const int &height)
{
  height_ = height;
}

const rational &VideoStream::timebase()
{
  return timebase_;
}

void VideoStream::set_timebase(const rational &timebase)
{
  timebase_ = timebase;
}

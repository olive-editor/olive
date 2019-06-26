#include "stream.h"

Stream::Stream() :
  footage_(nullptr)
{

}

Stream::~Stream()
{
}

Footage *Stream::footage()
{
  return footage_;
}

void Stream::set_footage(Footage *f)
{
  footage_ = f;
}

Stream::Type Stream::type()
{
  return kUnknown;
}

const rational &Stream::timebase()
{
  return timebase_;
}

void Stream::set_timebase(const rational &timebase)
{
  timebase_ = timebase;
}

const int &Stream::index()
{
  return index_;
}

void Stream::set_index(const int &index)
{
  index_ = index;
}

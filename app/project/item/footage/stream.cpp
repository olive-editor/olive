#include "stream.h"

Stream::Stream() :
  footage_(nullptr),
  type_(kUnknown)
{

}

Stream::~Stream()
{
}

const Stream::Type &Stream::type()
{
  return type_;
}

void Stream::set_type(const Stream::Type &type)
{
  type_ = type;
}

Footage *Stream::footage()
{
  return footage_;
}

void Stream::set_footage(Footage *f)
{
  footage_ = f;
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

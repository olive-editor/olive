#include "frame.h"

Frame::Frame() :
  frame_(nullptr)
{
}

Frame::Frame(AVFrame *f) :
  frame_(f)
{
}

Frame::Frame(const Frame &f) :
  frame_(nullptr)
{
}

Frame::Frame(Frame &&f) :
  frame_(f.frame_)
{
  f.frame_ = nullptr;
}

Frame &Frame::operator=(const Frame &f)
{
  frame_ = nullptr;
  return *this;
}

Frame &Frame::operator=(Frame &&f)
{
  if (&f != this) {
    frame_ = f.frame_;
    f.frame_ = nullptr;
  }

  return *this;
}

Frame::~Frame()
{
  FreeChild();
}

void Frame::SetAVFrame(AVFrame *f, AVRational timebase)
{
  FreeChild();

  f = frame_;
  timestamp_ = rational(timebase.num*f->pts, timebase.den);
}

const int &Frame::width()
{
  return frame_->width;
}

const int &Frame::height()
{
  return frame_->height;
}

const rational &Frame::timestamp()
{
  return timestamp_;
}

const int &Frame::format()
{
  return frame_->format;
}

uint8_t **Frame::data()
{
  return frame_->data;
}

int *Frame::linesize()
{
  return frame_->linesize;
}

void Frame::FreeChild()
{
  if (frame_ != nullptr) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }
}

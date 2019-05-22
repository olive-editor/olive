#include "memorycache.h"

#include <QDebug>

MemoryCache::MemoryCache() :
  ctx_(nullptr),
  width_(0),
  height_(0)
{

}

MemoryCache::~MemoryCache()
{
  buffer_array_mutex_.lock();

  Clear();

  buffer_array_mutex_.unlock();
}

void MemoryCache::SetParameters(QOpenGLContext *ctx, int width, int height)
{
  buffer_array_mutex_.lock();

  ctx_ = ctx;

  if (ctx_ != nullptr) {
    Q_ASSERT(width > 0 && height > 0);

    width_ = width;
    height_ = height;
  }

  Clear();

  buffer_array_mutex_.unlock();
}

int MemoryCache::RequestBuffer(Reference* r)
{
  if (ctx_ == nullptr) {
    qWarning() << "A memcache request was made with an invalid context";
    return -1;
  }

  if (width_ == 0 || height_ == 0) {
    qWarning() << "A memcache request was made with an invalid size [" << width_ << "," << height_ << "]";
    return -1;
  }

  buffer_array_mutex_.lock();

  int buffer = RequestBufferInternal(r);

  buffer_array_mutex_.unlock();

  return buffer;
}

int MemoryCache::RequestBufferInternal(MemoryCache::Reference *r)
{
  // Try to return a relinquished buffer
  if (!relinquished_buffers_.isEmpty()) {

    int buf_index = relinquished_buffers_.takeFirst();

    refs_.replace(buf_index, r);

    return buf_index;
  }

  // If no buffer was available, we may have to create a new one

  // Check if we have enough memory (according to user-defined limits in the Config), if we don't, try to relinquish an
  // old buffer and return that.
  if (OutOfMemory()) {

    int least_recent_access = 0;

    // Loop through buffers for the oldest accessed buffer
    for (int i=1;i<access_times_.size();i++) {
      if (access_times_.at(i) < access_times_.at(least_recent_access)
          && refs_.at(i) != nullptr) { // ensure this buffer has not been relinquished
        least_recent_access = i;
      }
    }

    // Relinquish this buffer
    refs_.at(least_recent_access)->Relinquish();
    refs_.replace(least_recent_access, r);
    return least_recent_access;
  }

  // Otherwise, just generate a new buffer
  buffers_.append(FramebufferObject());
  refs_.append(r);
  access_times_.append(time(nullptr));
  buffers_.last().Create(ctx_, width_, height_);

  return buffers_.size() - 1;
}

void MemoryCache::RelinquishBuffer(int index)
{
  buffer_array_mutex_.lock();

  refs_.replace(index, nullptr);
  relinquished_buffers_.append(index);

  buffer_array_mutex_.unlock();
}

FramebufferObject* MemoryCache::Buffer(int index)
{
  access_times_.replace(index, time(nullptr));
  return &buffers_[index];
}

bool MemoryCache::OutOfMemory()
{
  // TODO actually check if we have any memory or not
  return false;
}

void MemoryCache::Clear()
{
  for (int i=0;i<refs_.size();i++) {
    refs_.at(i)->Relinquish();
  }

  buffers_.clear();
  refs_.clear();
  relinquished_buffers_.clear();
}

MemoryCache::Reference::Reference(MemoryCache *cache) :
  cache_(cache),
  buffer_(-1)
{
}

MemoryCache::Reference::~Reference()
{
  Relinquish();
}

FramebufferObject *MemoryCache::Reference::buffer()
{
  // If we don't have a buffer yet, request one
  if (buffer_ == -1) {
    Request();
  }

  // If we didn't receive one, return null
  if (buffer_ == -1) {
    return nullptr;
  }

  // Otherwise, return the buffer we received
  return &cache_->buffers_[buffer_];
}

void MemoryCache::Reference::Relinquish()
{
  if (buffer_ == -1) {
    return;
  }

  cache_->RelinquishBuffer(buffer_);
  buffer_ = -1;
}

void MemoryCache::Reference::Request()
{
  // Check if we already have a buffer, in which case we don't need to request a new one
  if (buffer_ != -1) {
    return;
  }

  buffer_ = cache_->RequestBuffer(this);
}

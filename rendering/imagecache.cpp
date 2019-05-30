#include "imagecache.h"

#include <QDebug>

#include "decoders/pixelformatconverter.h"
#include "global/config.h"
#include "global/global.h"

ImageCache::ImageCache() :
  ctx_(nullptr),
  width_(0),
  height_(0)
{

}

ImageCache::~ImageCache()
{
  buffer_array_mutex_.lock();

  Clear();

  buffer_array_mutex_.unlock();
}

void ImageCache::SetParameters(QOpenGLContext *ctx, int width, int height)
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

int ImageCache::RequestBuffer(Ref* r, const BufferType& type, int width, int height)
{
  if (type == kTexBuf && (ctx_ == nullptr || width_ == 0 || height_ == 0)) {
    qWarning() << "A texture cache request was made without valid parameters [" << ctx_ << width_ << "," << height_ << "]";
    return -1;
  }

  buffer_array_mutex_.lock();

  int buffer = RequestBufferInternal(r, type, width, height);

  buffer_array_mutex_.unlock();

  return buffer;
}

int ImageCache::RequestBufferInternal(Ref *r, const BufferType& type, int width, int height)
{
  Q_ASSERT(type != kInvalidBuf);

  QList<int>& relinquished = (type == kMemBuf) ? img_relinquished_ : tex_relinquished_;
  QList<Ref*>& refs = (type == kMemBuf) ? img_refs_ : tex_refs_;
  QList<time_t>& access_times = (type == kMemBuf) ? img_access_times_ : tex_access_times_;

  // Try to return a relinquished buffer
  if (!relinquished.isEmpty()) {

    if (type == kTexBuf) {

      int buf_index = relinquished.takeFirst();

      refs.replace(buf_index, r);

      return buf_index;

    } else if (type == kMemBuf) {

      // Check for a relinquished buffer with the desired size

      //
      // TODO consider a database for sorting through these buffers
      //
      for (int i=0;i<relinquished.size();i++) {

        const MemoryBuffer& b = img_buffers_.at(relinquished.at(i));

        if (b.width() == width && b.height() == height) {
          // This buffer fits, we'll use it
          int buffer_index = relinquished.takeAt(i);

          refs.replace(buffer_index, r);

          return buffer_index;
        }
      }

    }
  }

  // If no buffer was available, we may have to create a new one

  // Check if we have enough memory (according to user-defined limits in the Config), if we don't, try to relinquish an
  // old buffer and return that.
  if (OutOfMemory()) {

    //
    // TODO no support for a MemBuf's variable size
    //

    int least_recent_access = 0;

    // Loop through buffers for the oldest accessed buffer
    for (int i=1;i<access_times.size();i++) {
      if (access_times.at(i) < access_times.at(least_recent_access)
          && refs.at(i) != nullptr) { // ensure this buffer has not been relinquished
        least_recent_access = i;
      }
    }

    // Relinquish this buffer
    refs.at(least_recent_access)->Relinquish();
    refs.replace(least_recent_access, r);
    return least_recent_access;
  }

  // Otherwise, just generate a new buffer
  int index = -1;
  switch (type) {
  case kMemBuf:
    index = img_buffers_.size();
    img_buffers_.append(MemoryBuffer());
    img_buffers_.last().Create(width_, height_, olive::Global->effective_bit_depth());
    break;
  case kTexBuf:
    index = tex_buffers_.size();
    tex_buffers_.append(FramebufferObject());
    tex_buffers_.last().Create(ctx_, width_, height_);
    break;
  default:
    Q_ASSERT(false);
  }

  refs.append(r);
  access_times.append(time(nullptr));

  return index;
}

void ImageCache::RelinquishBuffer(int index, const BufferType &type)
{
  buffer_array_mutex_.lock();

  switch (type) {
  case kMemBuf:
    img_refs_.replace(index, nullptr);
    img_relinquished_.append(index);
    break;
  case kTexBuf:
    tex_refs_.replace(index, nullptr);
    tex_relinquished_.append(index);
    break;
  default:
    qFatal("Invalid buffer type on ImageCache::RelinquishBuffer");
  }

  buffer_array_mutex_.unlock();
}

void *ImageCache::Buffer(int index, const BufferType &type)
{
  switch (type) {
  case kMemBuf:
    img_access_times_.replace(index, time(nullptr));
    return &img_buffers_[index];
  case kTexBuf:
    tex_access_times_.replace(index, time(nullptr));
    return &tex_buffers_[index];
  default:
    qFatal("Invalid buffer type on ImageCache::Buffer");
  }
}

bool ImageCache::OutOfMemory()
{
  // TODO actually check if we have any memory or not
  return false;
}

void ImageCache::Clear()
{
  for (int i=0;i<img_refs_.size();i++) {
    img_refs_.at(i)->Relinquish();
  }

  for (int i=0;i<tex_refs_.size();i++) {
    tex_refs_.at(i)->Relinquish();
  }

  img_refs_.clear();
  tex_refs_.clear();

  img_buffers_.clear();
  tex_buffers_.clear();

  img_relinquished_.clear();
  tex_relinquished_.clear();
}

ImageCache::Ref::Ref(ImageCache *cache) :
  buffer_type_(kInvalidBuf),
  cache_(cache),
  buffer_(-1),
  width_(-1),
  height_(-1)
{
}

ImageCache::Ref::~Ref()
{
  Relinquish();
}

void *ImageCache::Ref::BufferInternal()
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
  return cache_->Buffer(buffer_, buffer_type_);
}

void ImageCache::Ref::Relinquish()
{
  if (buffer_ == -1) {
    return;
  }

  cache_->RelinquishBuffer(buffer_, buffer_type_);
  buffer_ = -1;
}

void ImageCache::Ref::Request()
{
  // Check if we already have a buffer, in which case we don't need to request a new one
  if (buffer_ != -1) {
    return;
  }

  // Check if we have a valid buffer type to request
  if (buffer_type_ == kInvalidBuf) {
    qWarning() << "Tried to request an invalid buffer type";
    return;
  }

  buffer_ = cache_->RequestBuffer(this, buffer_type_, width_, height_);
}

ImageCache::ImgRef::ImgRef(ImageCache *cache) :
  Ref(cache)
{
  buffer_type_ = kMemBuf;
}

void ImageCache::ImgRef::SetSize(int w, int h)
{
  width_ = w;
  height_ = h;

  Relinquish();
}

MemoryBuffer *ImageCache::ImgRef::buffer()
{
  return static_cast<MemoryBuffer*>(BufferInternal());
}

ImageCache::TexRef::TexRef(ImageCache *cache) :
  Ref(cache)
{
  buffer_type_ = kTexBuf;
}

FramebufferObject *ImageCache::TexRef::buffer()
{
  return static_cast<FramebufferObject*>(BufferInternal());
}

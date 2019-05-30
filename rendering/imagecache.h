#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include <QList>
#include <QMutex>

#include "framebufferobject.h"
#include "memorybuffer.h"

class ImageCache
{
public:

  enum BufferType {
    kInvalidBuf,
    kMemBuf,  // RAM (CPU) buffer
    kTexBuf   // VRAM (GPU) buffer
  };

  class Ref {
  public:
    Ref(ImageCache* cache);
    ~Ref();

    void Relinquish();
  protected:
    void* BufferInternal();

    BufferType buffer_type_;

    int width_;
    int height_;
  private:
    void Request();

    ImageCache* cache_;
    int buffer_;
  };

  class ImgRef : public Ref {
  public:
    ImgRef(ImageCache* cache);

    void SetSize(int w, int h);

    MemoryBuffer* buffer();

  };

  class TexRef : public Ref {
  public:
    TexRef(ImageCache* cache);
    FramebufferObject* buffer();
  };

  ImageCache();
  ~ImageCache();

  void SetParameters(QOpenGLContext* ctx, int width, int height);

private:
  int RequestBuffer(Ref *r, const BufferType& type, int width, int height);
  int RequestBufferInternal(Ref *r, const BufferType& type, int width, int height);

  void RelinquishBuffer(int index, const BufferType& type);

  void *Buffer(int index, const BufferType &type);

  bool OutOfMemory();

  void Clear();

  QList<MemoryBuffer> img_buffers_;
  QList<Ref*> img_refs_;
  QList<time_t> img_access_times_;
  QList<int> img_relinquished_;

  QList<FramebufferObject> tex_buffers_;
  QList<Ref*> tex_refs_;
  QList<time_t> tex_access_times_;
  QList<int> tex_relinquished_;

  QOpenGLContext* ctx_;

  int width_;
  int height_;

  QMutex buffer_array_mutex_;
};

#endif // MEMORYCACHE_H

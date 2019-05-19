#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include <QList>
#include <QMutex>

#include "framebufferobject.h"

class MemoryCache
{
public:
  class Reference {
  public:
    Reference(MemoryCache* cache);
    ~Reference();

    FramebufferObject* buffer();

    void Relinquish();
  private:
    void Request();

    MemoryCache* cache_;
    int buffer_;
  };

  MemoryCache();
  ~MemoryCache();

  void SetParameters(QOpenGLContext* ctx, int width, int height);

private:
  int RequestBuffer(Reference *r);
  int RequestBufferInternal(Reference *r);

  void RelinquishBuffer(int index);

  FramebufferObject *Buffer(int index);

  bool OutOfMemory();

  void Clear();

  QList<FramebufferObject> buffers_;
  QList<Reference*> refs_;
  QList<time_t> access_times_;

  QOpenGLContext* ctx_;

  QList<int> relinquished_buffers_;

  int width_;
  int height_;

  QMutex buffer_array_mutex_;
};

#endif // MEMORYCACHE_H

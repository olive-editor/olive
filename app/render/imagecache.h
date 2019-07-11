/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include <QList>
#include <QMutex>

#include "texturebuffer.h"
#include "memorybuffer.h"

class ImageCache : public QObject
{
public:

  enum BufferType {
    kInvalidBuf,  // No buffer
    kMemBuf,      // RAM (CPU) buffer
    kTexBuf       // VRAM (GPU) buffer
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
    TextureBuffer* buffer();
  };

  ImageCache();
  ~ImageCache();

  void SetParameters(QOpenGLContext* ctx, int width, int height);

private:
  int RequestBuffer(Ref *r, const BufferType& type, const olive::PixelFormat &format, int width, int height);
  int RequestBufferInternal(Ref *r, const BufferType& type, const olive::PixelFormat &format, int width, int height);

  void RelinquishBuffer(int index, const BufferType& type);

  void *Buffer(int index, const BufferType &type);

  bool OutOfMemory();

  void Clear();

  QList<MemoryBuffer> img_buffers_;
  QList<Ref*> img_refs_;
  QList<time_t> img_access_times_;
  QList<int> img_relinquished_;

  QList<TextureBuffer> tex_buffers_;
  QList<Ref*> tex_refs_;
  QList<time_t> tex_access_times_;
  QList<int> tex_relinquished_;

  QOpenGLContext* ctx_;

  int width_;
  int height_;

  QMutex buffer_array_mutex_;
};

#endif // MEMORYCACHE_H

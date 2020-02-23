#ifndef FFMPEGFRAMECACHE_H
#define FFMPEGFRAMECACHE_H

#include <QList>
#include <QMutex>

#include "codec/frame.h"
#include "render/videoparams.h"

class FFmpegFrameCache
{
public:
  FFmpegFrameCache() = default;

  static Frame* Get(const VideoRenderingParams& params);

  static void Release(Frame* f);

  class Client
  {
  public:
    Client() = default;

    Frame* append(const VideoRenderingParams &params);
    void clear();

    bool isEmpty() const;
    Frame* first() const;
    Frame* at(int i) const;
    Frame* last() const;
    int size() const;

    void accessedFirst();
    void accessedLast();
    void accessed(int i);

    void remove_old_frames(qint64 older_than);

  private:
    struct CachedFrame {
      Frame* frame;
      qint64 accessed;
    };

    QList<CachedFrame> frames_;

  };

private:
  static QMutex pool_lock_;

  static QList<Frame*> frame_pool_;

};

#endif // FFMPEGFRAMECACHE_H

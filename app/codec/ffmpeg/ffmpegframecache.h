#ifndef FFMPEGFRAMECACHE_H
#define FFMPEGFRAMECACHE_H

extern "C" {
#include <libavformat/avformat.h>
}

#include <QList>
#include <QTimer>

class FFmpegFrameCache
{
public:
  FFmpegFrameCache();

  void append(AVFrame* f);
  void clear();

  bool isEmpty() const;
  AVFrame* first() const;
  AVFrame* at(int i) const;
  AVFrame* last() const;
  int size() const;

  void accessedFirst();
  void accessedLast();
  void accessed(int i);

  void remove_old_frames(qint64 older_than);

private:
  struct CachedFrame {
    AVFrame* frame;
    qint64 accessed;
  };

  QList<CachedFrame> frames_;

  static int global_frame_count_;

};

#endif // FFMPEGFRAMECACHE_H

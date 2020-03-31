#include "ffmpegframecache.h"

#include <QDateTime>
#include <QDebug>

QMutex FFmpegFrameCache::pool_lock_;
QList<Frame*> FFmpegFrameCache::frame_pool_;

Frame *FFmpegFrameCache::Client::append(const VideoRenderingParams& params)
{
  Frame* f = FFmpegFrameCache::Get(params);

  frames_.append({f, QDateTime::currentMSecsSinceEpoch()});

  return f;
}

void FFmpegFrameCache::Client::clear()
{
  foreach (const CachedFrame& cf, frames_) {
    FFmpegFrameCache::Release(cf.frame);
  }
  frames_.clear();
}

bool FFmpegFrameCache::Client::isEmpty() const
{
  return frames_.isEmpty();
}

Frame *FFmpegFrameCache::Client::first() const
{
  return frames_.first().frame;
}

Frame *FFmpegFrameCache::Client::at(int i) const
{
  return frames_.at(i).frame;
}

Frame *FFmpegFrameCache::Client::last() const
{
  return frames_.last().frame;
}

int FFmpegFrameCache::Client::size() const
{
  return frames_.size();
}

void FFmpegFrameCache::Client::accessedFirst()
{
  frames_.first().accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::Client::accessedLast()
{
  frames_.last().accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::Client::accessed(int i)
{
  frames_[i].accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::Client::remove_old_frames(qint64 older_than)
{
  while (!frames_.isEmpty() && frames_.first().accessed < older_than) {
    FFmpegFrameCache::Release(frames_.takeFirst().frame);
  }
}

Frame* FFmpegFrameCache::Get(const VideoRenderingParams &params)
{
  QMutexLocker locker(&pool_lock_);

  // See if we have a frame matching this description in the pool
  for (int i=0;i<frame_pool_.size();i++) {
    if (frame_pool_.at(i)->width() == params.width()
        && frame_pool_.at(i)->height() == params.height()
        && frame_pool_.at(i)->format() == params.format()) {
      return frame_pool_.takeAt(i);
    }
  }

  // Otherwise we'll need to create one
  Frame* f = new Frame();
  f->set_video_params(params);
  f->allocate();

  return f;
}

void FFmpegFrameCache::Release(Frame *f)
{
  QMutexLocker locker(&pool_lock_);

  frame_pool_.append(f);
}

#include "ffmpegframecache.h"

#include <QDateTime>
#include <QDebug>

int FFmpegFrameCache::global_frame_count_ = 0;

FFmpegFrameCache::FFmpegFrameCache()
{

}

void FFmpegFrameCache::append(AVFrame *f)
{
  global_frame_count_++;

  frames_.append({f, QDateTime::currentMSecsSinceEpoch()});
}

void FFmpegFrameCache::clear()
{
  global_frame_count_ -= frames_.size();

  frames_.clear();
}

bool FFmpegFrameCache::isEmpty() const
{
  return frames_.isEmpty();
}

AVFrame *FFmpegFrameCache::first() const
{
  return frames_.first().frame;
}

AVFrame *FFmpegFrameCache::at(int i) const
{
  return frames_.at(i).frame;
}

AVFrame *FFmpegFrameCache::last() const
{
  return frames_.last().frame;
}

int FFmpegFrameCache::size() const
{
  return frames_.size();
}

void FFmpegFrameCache::accessedFirst()
{
  frames_.first().accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::accessedLast()
{
  frames_.last().accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::accessed(int i)
{
  frames_[i].accessed = QDateTime::currentMSecsSinceEpoch();
}

void FFmpegFrameCache::remove_old_frames(qint64 older_than)
{
  int counter = 0;

  while (!frames_.isEmpty() && frames_.first().accessed < older_than) {
    CachedFrame cf = frames_.takeFirst();
    av_frame_free(&cf.frame);
    counter++;
  }

  qDebug() << "  * Removed" << counter << "frames";
  global_frame_count_ -= counter;
  qDebug() << "  * Global frames:" << global_frame_count_;
}

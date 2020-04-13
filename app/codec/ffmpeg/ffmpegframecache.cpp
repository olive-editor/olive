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

#include "ffmpegframecache.h"

#include <QDateTime>
#include <QDebug>

OLIVE_NAMESPACE_ENTER

QMutex FFmpegFrameCache::pool_lock_;
QLinkedList<AVFramePtr> FFmpegFrameCache::frame_pool_;

AVFramePtr FFmpegFrameCache::Client::append(int width, int height, int format)
{
  AVFramePtr f = FFmpegFrameCache::Get(width, height, format);

  frames_.append(f);

  return f;
}

AVFramePtr FFmpegFrameCache::Client::append(AVFrame *copy)
{
  AVFramePtr f = append(copy->width, copy->height, copy->format);

  av_frame_copy(f->frame(), copy);
  f->frame()->pts = copy->pts;

  return f;
}

void FFmpegFrameCache::Client::clear()
{
  foreach (AVFramePtr cf, frames_) {
    FFmpegFrameCache::Release(cf);
  }
  frames_.clear();
}

bool FFmpegFrameCache::Client::isEmpty() const
{
  return frames_.isEmpty();
}

AVFramePtr FFmpegFrameCache::Client::first() const
{
  return frames_.first();
}

AVFramePtr FFmpegFrameCache::Client::at(int i) const
{
  return frames_.at(i);
}

AVFramePtr FFmpegFrameCache::Client::last() const
{
  return frames_.last();
}

int FFmpegFrameCache::Client::size() const
{
  return frames_.size();
}

void FFmpegFrameCache::Client::accessedFirst()
{
  frames_.first()->access();
}

void FFmpegFrameCache::Client::accessedLast()
{
  frames_.last()->access();
}

void FFmpegFrameCache::Client::accessed(int i)
{
  frames_[i]->access();
}

int FFmpegFrameCache::Client::remove_old_frames(qint64 older_than)
{
  int counter = 0;

  while (frames_.size() > 1 && frames_.first()->last_accessed() < older_than) {
    FFmpegFrameCache::Release(frames_.takeFirst());
    counter++;
  }

  return counter;
}

AVFramePtr FFmpegFrameCache::Get(int width, int height, int format)
{
  QMutexLocker locker(&pool_lock_);

  // See if we have a frame matching this description in the pool
  QLinkedList<AVFramePtr>::iterator i;
  for (i=frame_pool_.begin();i!=frame_pool_.end();i++) {
    AVFramePtr f = (*i);

    if (f->frame()->width == width
        && f->frame()->height == height
        && f->frame()->format == format) {
      frame_pool_.erase(i);
      return f;
    }
  }

  // Otherwise we'll need to create one
  AVFramePtr f = std::make_shared<AVFrameWrapper>();
  f->frame()->width = width;
  f->frame()->height = height;
  f->frame()->format = format;
  av_frame_get_buffer(f->frame(), 1);

  return f;
}

void FFmpegFrameCache::Release(AVFramePtr f)
{
  QMutexLocker locker(&pool_lock_);

  frame_pool_.append(f);
}

OLIVE_NAMESPACE_EXIT

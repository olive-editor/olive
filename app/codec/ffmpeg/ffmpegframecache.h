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

#ifndef FFMPEGFRAMECACHE_H
#define FFMPEGFRAMECACHE_H

#include <QList>
#include <QMutex>

#include "codec/frame.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT

#endif // FFMPEGFRAMECACHE_H

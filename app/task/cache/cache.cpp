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

#include "cache.h"

#include <QLinkedList>
#include <QMatrix4x4>

#include "project/item/sequence/sequence.h"

OLIVE_NAMESPACE_ENTER

CacheTask::CacheTask(ViewerOutput* viewer, int divider, bool in_out_only) :
  RenderTask(viewer),
  in_out_only_(in_out_only),
  divider_(divider)
{
  SetTitle(tr("Caching \"%1\"").arg(viewer->media_name()));
}

bool CacheTask::Run()
{
  // Get list of invalidated ranges
  TimeRangeList range_to_cache = viewer()->video_frame_cache()->GetInvalidatedRanges();

  // If we're caching only in-out, limit the range to that
  if (in_out_only_) {
    Sequence* s = static_cast<Sequence*>(viewer()->parent());

    if (s->workarea()->enabled()) {
      range_to_cache = range_to_cache.Intersects(s->workarea()->range());
    }
  }

  Render(range_to_cache, RenderMode::kOffline, QMatrix4x4(), false, divider_);

  return true;
}

QFuture<void> CacheTask::DownloadFrame(FramePtr frame, const QByteArray &hash)
{
  return QtConcurrent::run(FrameHashCache::SaveCacheFrame, hash, frame);
}

void CacheTask::FrameDownloaded(const QByteArray &hash, const QLinkedList<rational> &times)
{
  foreach (const rational& t, times) {
    viewer()->video_frame_cache()->SetHash(t, hash);
  }
}

OLIVE_NAMESPACE_EXIT

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "playbackcache.h"

#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/sequence/sequence.h"
#include "render/diskmanager.h"

namespace olive {

void PlaybackCache::Invalidate(const TimeRange &r)
{
  if (r.in() == r.out()) {
    qWarning() << "Tried to invalidate zero-length range";
    return;
  }

  validated_.remove(r);

  if (!passthroughs_.empty()) {
    TimeRangeList::util_remove(&passthroughs_, r);
  }

  InvalidateEvent(r);

  emit Invalidated(r);

  if (saving_enabled_) {
    SaveState();
  }
}

Node *PlaybackCache::parent() const
{
  return dynamic_cast<Node*>(QObject::parent());
}

QDir PlaybackCache::GetThisCacheDirectory() const
{
  return GetThisCacheDirectory(GetCacheDirectory(), GetUuid());
}

QDir PlaybackCache::GetThisCacheDirectory(const QString &cache_path, const QUuid &cache_id)
{
  return QDir(cache_path).filePath(cache_id.toString());
}

void PlaybackCache::LoadState()
{
  QDir cache_dir = GetThisCacheDirectory();
  QFile f(cache_dir.filePath(QStringLiteral("state")));

  if (!f.exists()) {
    // No state exists, assume nothing valid
    validated_.clear();
    passthroughs_.clear();
    return;
  }

  qint64 file_time = f.fileTime(QFileDevice::FileModificationTime).toMSecsSinceEpoch();
  if (file_time > last_loaded_state_ && f.open(QFile::ReadOnly)) {
    QDataStream s(&f);

    uint32_t version;
    s >> version;

    LoadStateEvent(s);

    switch (version) {
    case 1:
    {
      int valid_count, pass_count;

      s >> valid_count;
      for (int i=0; i<valid_count; i++) {
        int in_num, in_den, out_num, out_den;

        s >> in_num;
        s >> in_den;
        s >> out_num;
        s >> out_den;

        validated_.insert(TimeRange(rational(in_num, in_den), rational(out_num, out_den)));
      }

      s >> pass_count;
      for (int i=0; i<pass_count; i++) {
        QUuid id;
        int in_num, in_den, out_num, out_den;

        s >> in_num;
        s >> in_den;
        s >> out_num;
        s >> out_den;
        s >> id;

        Passthrough p = TimeRange(rational(in_num, in_den), rational(out_num, out_den));
        p.cache = id;
        passthroughs_.push_back(p);
      }

      break;
    }
    }

    f.close();

    last_loaded_state_ = file_time;
  }
}

void PlaybackCache::SaveState()
{
  if (!DiskManager::instance()) {
    return;
  }

  QDir cache_dir = GetThisCacheDirectory();
  QFile f(cache_dir.filePath(QStringLiteral("state")));
  if (validated_.isEmpty() && passthroughs_.empty()) {
    if (f.exists()) {
      f.remove();
    }
  } else {
    if (FileFunctions::DirectoryIsValid(cache_dir)) {
      if (f.open(QFile::WriteOnly)) {
        QDataStream s(&f);

        uint32_t version = 1;
        s << version;

        SaveStateEvent(s);

        // Using "int" for backwards compatibility with when we used QVector, could potentially overflow
        s << int(validated_.size());

        for (const TimeRange &r : validated_) {
          s << r.in().numerator();
          s << r.in().denominator();
          s << r.out().numerator();
          s << r.out().denominator();
        }

        // Using "int" for backwards compatibility with when we used QVector, could potentially overflow
        s << int(passthroughs_.size());

        for (const Passthrough &p : passthroughs_) {
          s << p.in().numerator();
          s << p.in().denominator();
          s << p.out().numerator();
          s << p.out().denominator();
          s << p.cache;
        }

        f.close();

        last_loaded_state_ = f.fileTime(QFileDevice::FileModificationTime).toMSecsSinceEpoch();
      }
    }
  }
}

void PlaybackCache::Draw(QPainter *p, const rational &start, double scale, const QRect &rect) const
{
  p->fillRect(rect, Qt::red);

  foreach (const TimeRange& range, GetValidatedRanges()) {
    int range_left = rect.left() + (range.in() - start).toDouble() * scale;
    if (range_left >= rect.right()) {
      continue;
    }

    int range_right = rect.left() + (range.out() - start).toDouble() * scale;
    if (range_right < rect.left()) {
      continue;
    }

    int adjusted_left = std::max(range_left, rect.left());
    int adjusted_right = std::min(range_right, rect.right());

    p->fillRect(adjusted_left,
                rect.top(),
                adjusted_right - adjusted_left,
                rect.height(),
                Qt::green);
  }
}

void PlaybackCache::SetPassthrough(PlaybackCache *cache)
{
  for (const TimeRange &r : cache->GetValidatedRanges()) {
    Passthrough p = r;
    p.cache = cache->GetUuid();
    passthroughs_.push_back(p);
  }

  passthroughs_.insert(passthroughs_.end(), cache->GetPassthroughs().begin(), cache->GetPassthroughs().end());

  if (saving_enabled_) {
    SaveState();
  }
}

void PlaybackCache::InvalidateAll()
{
  Invalidate(TimeRange(0, RATIONAL_MAX));
}

void PlaybackCache::Request(ViewerOutput *context, const TimeRange &r)
{
  request_context_ = context;
  requested_.insert(r);

  emit Requested(request_context_, r);
}

void PlaybackCache::Validate(const TimeRange &r, bool signal)
{
  validated_.insert(r);

  if (signal) {
    emit Validated(r);
  }

  if (saving_enabled_) {
    SaveState();
  }
}

void PlaybackCache::InvalidateEvent(const TimeRange &)
{
}

Project *PlaybackCache::GetProject() const
{
  return Project::GetProjectFromObject(this);
}

PlaybackCache::PlaybackCache(QObject *parent) :
  QObject(parent),
  saving_enabled_(true),
  last_loaded_state_(0)
{
  uuid_ = QUuid::createUuid();
}

void PlaybackCache::SetUuid(const QUuid &u)
{
  uuid_ = u;

  LoadState();
}

TimeRangeList PlaybackCache::GetInvalidatedRanges(TimeRange intersecting) const
{
  TimeRangeList invalidated;

  // Prevent TimeRange from being below 0, some other behavior in Olive relies on this behavior
  // and it seemed reasonable to have safety code in here
  intersecting.set_out(qMax(rational(0), intersecting.out()));
  intersecting.set_in(qMax(rational(0), intersecting.in()));

  invalidated.insert(intersecting);

  foreach (const TimeRange &range, validated_) {
    invalidated.remove(range);
  }

  foreach (const TimeRange &range, passthroughs_) {
    invalidated.remove(range);
  }

  return invalidated;
}

bool PlaybackCache::HasInvalidatedRanges(const TimeRange &intersecting) const
{
  return !validated_.contains(intersecting);
}

QString PlaybackCache::GetCacheDirectory() const
{
  Project* project = GetProject();

  if (project) {
    return project->cache_path();
  } else {
    return DiskManager::instance()->GetDefaultCachePath();
  }
}

}

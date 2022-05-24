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
#include "node/project/project.h"
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

  InvalidateEvent(r);

  emit Invalidated(r);

  if (automatic_) {
    emit Request(r, false);
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

void PlaybackCache::InvalidateAll()
{
  Invalidate(TimeRange(0, RATIONAL_MAX));
}

void PlaybackCache::Validate(const TimeRange &r, bool signal)
{
  validated_.insert(r);

  if (signal) {
    emit Validated(r);
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
  automatic_(false)
{
  uuid_ = QUuid::createUuid();
}

void PlaybackCache::SetIsAutomatic(bool e)
{
  if (automatic_ != e) {
    automatic_ = e;

    emit AutomaticChanged(automatic_);
  }
}

TimeRangeList PlaybackCache::GetInvalidatedRanges(TimeRange intersecting)
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

  return invalidated;
}

bool PlaybackCache::HasInvalidatedRanges(const TimeRange &intersecting)
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

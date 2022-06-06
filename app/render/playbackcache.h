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

#ifndef PLAYBACKCACHE_H
#define PLAYBACKCACHE_H

#include <QDir>
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QUuid>

#include "common/jobtime.h"
#include "common/timerange.h"

namespace olive {

class Node;
class Project;
class ViewerOutput;

class PlaybackCache : public QObject
{
  Q_OBJECT
public:
  PlaybackCache(QObject* parent = nullptr);

  const QUuid &GetUuid() const { return uuid_; }
  void SetUuid(const QUuid &u);

  TimeRangeList GetInvalidatedRanges(TimeRange intersecting) const;
  TimeRangeList GetInvalidatedRanges(const rational &length) const
  {
    return GetInvalidatedRanges(TimeRange(0, length));
  }

  bool HasInvalidatedRanges(const TimeRange &intersecting) const;
  bool HasInvalidatedRanges(const rational &length) const
  {
    return HasInvalidatedRanges(TimeRange(0, length));
  }

  QString GetCacheDirectory() const;

  void Invalidate(const TimeRange& r);

  bool HasValidatedRanges() const { return !validated_.isEmpty(); }
  const TimeRangeList &GetValidatedRanges() const { return validated_; }

  Node *parent() const;

  QDir GetThisCacheDirectory() const;
  static QDir GetThisCacheDirectory(const QString &cache_path, const QUuid &cache_id);

  void LoadState();
  void SaveState();

  void Draw(QPainter *painter, const rational &start, double scale, const QRect &rect) const;

  static int GetCacheIndicatorHeight()
  {
    return QFontMetrics(QFont()).height()/4;
  }

  QMutex *mutex() { return &mutex_; }

public slots:
  void InvalidateAll();

signals:
  void Invalidated(const olive::TimeRange& r);

  void Validated(const olive::TimeRange& r);

  void Request(const olive::TimeRange& r);

  void CancelAll();

protected:
  void Validate(const TimeRange& r, bool signal = true);

  virtual void InvalidateEvent(const TimeRange& range);

  virtual void LoadStateEvent(QDataStream &stream){}

  virtual void SaveStateEvent(QDataStream &stream){}

  Project* GetProject() const;

private:
  TimeRangeList validated_;

  QUuid uuid_;

  QMutex mutex_;

};

}

#endif // PLAYBACKCACHE_H

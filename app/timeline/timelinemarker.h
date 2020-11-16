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

#ifndef TIMELINEMARKER_H
#define TIMELINEMARKER_H

#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/timerange.h"

OLIVE_NAMESPACE_ENTER

class TimelineMarker : public QObject
{
  Q_OBJECT
public:
  TimelineMarker(const TimeRange& time = TimeRange(), const QString& name = QString(), QObject* parent = nullptr);

  const TimeRange &time() const;
  void set_time(const TimeRange& time);

  const QString& name() const;
  void set_name(const QString& name);

signals:
  void TimeChanged(const TimeRange& time);

  void NameChanged(const QString& name);

private:
  TimeRange time_;

  QString name_;

};

class TimelineMarkerList : public QObject
{
  Q_OBJECT
public:
  TimelineMarkerList() = default;

  virtual ~TimelineMarkerList() override;

  TimelineMarker *AddMarker(const TimeRange& time = TimeRange(), const QString& name = QString());

  void RemoveMarker(TimelineMarker* marker);

  const QList<TimelineMarker *> &list() const;

  void Load(QXmlStreamReader* reader);
  void Save(QXmlStreamWriter* writer) const;

signals:
  void MarkerAdded(TimelineMarker* marker);

  void MarkerRemoved(TimelineMarker* marker);

private:
  QList<TimelineMarker*> markers_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEMARKER_H

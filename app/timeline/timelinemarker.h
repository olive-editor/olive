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

#ifndef TIMELINEMARKER_H
#define TIMELINEMARKER_H

#include <olive/core/core.h>
#include <QPainter>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "undo/undocommand.h"

using namespace olive::core;

namespace olive {

class TimelineMarker : public QObject
{
  Q_OBJECT
public:
  TimelineMarker(QObject* parent = nullptr);
  TimelineMarker(int color, const TimeRange& time, const QString& name = QString(), QObject* parent = nullptr);

  const TimeRange &time() const { return time_; }
  void set_time(const TimeRange& time);
  void set_time(const rational& time);

  bool has_sibling_at_time(const rational &t) const;

  const QString& name() const { return name_; }
  void set_name(const QString& name);

  int color() const { return color_; }
  void set_color(int c);

  static int GetMarkerHeight(const QFontMetrics &fm);
  QRect Draw(QPainter *p, const QPoint &pt, int max_right, double scale, bool selected);

  bool load(QXmlStreamReader *reader);
  void save(QXmlStreamWriter *writer) const;

signals:
  void TimeChanged(const TimeRange& time);

  void NameChanged(const QString& name);

  void ColorChanged(int c);

private:
  TimeRange time_;

  QString name_;

  int color_;

};

class TimelineMarkerList : public QObject
{
  Q_OBJECT
public:
  TimelineMarkerList(QObject *parent = nullptr) :
    QObject(parent)
  {
  }

  inline bool empty() const { return markers_.empty(); }
  inline std::vector<TimelineMarker*>::iterator begin() { return markers_.begin(); }
  inline std::vector<TimelineMarker*>::iterator end() { return markers_.end(); }
  inline std::vector<TimelineMarker*>::const_iterator cbegin() const { return markers_.cbegin(); }
  inline std::vector<TimelineMarker*>::const_iterator cend() const { return markers_.cend(); }
  inline TimelineMarker *back() const { return markers_.back(); }
  inline TimelineMarker *front() const { return markers_.front(); }
  inline size_t size() const { return markers_.size(); }

  bool load(QXmlStreamReader *reader);
  void save(QXmlStreamWriter *writer) const;

  TimelineMarker *GetMarkerAtTime(const rational &t) const
  {
    for (auto it=markers_.cbegin(); it!=markers_.cend(); it++) {
      TimelineMarker *m = *it;
      if (m->time().in() == t) {
        return m;
      }
    }

    return nullptr;
  }

  TimelineMarker *GetClosestMarkerToTime(const rational &t) const
  {
    TimelineMarker *closest = nullptr;

    for (auto it=markers_.cbegin(); it!=markers_.cend(); it++) {
      TimelineMarker *m = *it;

      rational this_diff = qAbs(m->time().in() - t);

      if (closest) {
        rational stored_diff = qAbs(closest->time().in() - t);

        if (this_diff > stored_diff) {
          // Since the list is organized by time, if the diff increases, assume we are only going
          // to move further away from here and there's no need to check
          break;
        }
      }

      closest = m;
    }

    return closest;
  }

signals:
  void MarkerAdded(TimelineMarker* marker);

  void MarkerRemoved(TimelineMarker* marker);

  void MarkerModified(TimelineMarker* marker);

protected:
  virtual void childEvent(QChildEvent *e) override;

private:
  void InsertIntoList(TimelineMarker *m);
  bool RemoveFromList(TimelineMarker *m);

  std::vector<TimelineMarker*> markers_;

private slots:
  void HandleMarkerModification();

  void HandleMarkerTimeChange();

};

class MarkerAddCommand : public UndoCommand {
public:
  MarkerAddCommand(TimelineMarkerList* marker_list, const TimeRange& range, const QString& name, int color);
  MarkerAddCommand(TimelineMarkerList* marker_list, TimelineMarker *marker);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarkerList* marker_list_;

  TimelineMarker* added_marker_;
  QObject memory_manager_;

};

class MarkerRemoveCommand : public UndoCommand {
public:
  MarkerRemoveCommand(TimelineMarker* marker);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarker* marker_;
  QObject* marker_list_;

  QObject memory_manager_;

};

class MarkerChangeColorCommand : public UndoCommand {
public:
  MarkerChangeColorCommand(TimelineMarker* marker, int new_color);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarker* marker_;
  int old_color_;
  int new_color_;

};

class MarkerChangeNameCommand : public UndoCommand {
public:
  MarkerChangeNameCommand(TimelineMarker* marker, QString name);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarker* marker_;
  QString old_name_;
  QString new_name_;
};

class MarkerChangeTimeCommand : public UndoCommand {
public:
  MarkerChangeTimeCommand(TimelineMarker* marker, const TimeRange &time, const TimeRange &old_time);
  MarkerChangeTimeCommand(TimelineMarker* marker, const TimeRange &time) :
    MarkerChangeTimeCommand(marker, time, marker->time())
  {}

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarker* marker_;
  TimeRange old_time_;
  TimeRange new_time_;

};

}

#endif // TIMELINEMARKER_H

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "undo/undocommand.h"

namespace olive {

class TimelineMarker : public QObject
{
  Q_OBJECT
public:
  TimelineMarker(int color, const TimeRange& time, const QString& name = QString(), QObject* parent = nullptr);

  const TimeRange &time() const { return time_; }
  void set_time(const TimeRange& time);

  const QString& name() const { return name_; }
  void set_name(const QString& name);

  int color() const { return color_; }
  void set_color(int c);

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

  const std::vector<TimelineMarker *> &list() const;

signals:
  void MarkerAdded(TimelineMarker* marker);

  void MarkerRemoved(TimelineMarker* marker);

protected:
  virtual void childEvent(QChildEvent *e) override;

private:
  std::vector<TimelineMarker*> markers_;

};

class MarkerAddCommand : public UndoCommand {
public:
  MarkerAddCommand(TimelineMarkerList* marker_list, const TimeRange& range, const QString& name, int color);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo() override;
  virtual void undo() override;

private:
  TimelineMarkerList* marker_list_;
  TimeRange range_;
  QString name_;
  int color_;

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
  TimeRange range_;
  QString name_;
  int color_;

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
  MarkerChangeTimeCommand(TimelineMarker* marker, TimeRange time);

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

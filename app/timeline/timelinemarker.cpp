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

#include "timelinemarker.h"

#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"

namespace olive {

TimelineMarker::TimelineMarker(int color, const TimeRange &time, const QString &name, QObject *parent) :
  QObject(parent),
  time_(time),
  name_(name),
  color_(color)
{
}

void TimelineMarker::set_time(const TimeRange &time)
{
  time_ = time;
  emit TimeChanged(time_);
}

void TimelineMarker::set_name(const QString &name)
{
  name_ = name;
  emit NameChanged(name_);
}

void TimelineMarker::set_color(int c)
{
  color_ = c;

  emit ColorChanged(color_);
}

const std::vector<TimelineMarker*> &TimelineMarkerList::list() const
{
  return markers_;
}

void TimelineMarkerList::childEvent(QChildEvent *e)
{
  QObject::childEvent(e);

  if (TimelineMarker *marker = dynamic_cast<TimelineMarker *>(e->child())) {
    if (e->type() == QChildEvent::ChildAdded) {
      markers_.push_back(marker);
    } else if (e->type() == QChildEvent::ChildRemoved) {
      auto it = std::find(markers_.begin(), markers_.end(), marker);
      if (it != markers_.end()) {
        markers_.erase(it);
      }
    }
  }
}

MarkerAddCommand::MarkerAddCommand(TimelineMarkerList *marker_list, const TimeRange &range, const QString &name, int color) :
  marker_list_(marker_list)
{
  added_marker_ = new TimelineMarker(color, range, name, &memory_manager_);
}

Project* MarkerAddCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_list_);
}

void MarkerAddCommand::redo()
{
  added_marker_->setParent(marker_list_);
}

void MarkerAddCommand::undo()
{
  added_marker_->setParent(&memory_manager_);
}

MarkerRemoveCommand::MarkerRemoveCommand(TimelineMarker *marker) :
  marker_(marker)
{
}

Project* MarkerRemoveCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_list_);
}

void MarkerRemoveCommand::redo()
{
  marker_list_ = marker_->parent();
  marker_->setParent(&memory_manager_);
}

void MarkerRemoveCommand::undo()
{
  marker_->setParent(marker_list_);
}

MarkerChangeColorCommand::MarkerChangeColorCommand(TimelineMarker *marker, int new_color) :
  marker_(marker),
  new_color_(new_color)
{
}

Project* MarkerChangeColorCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeColorCommand::redo()
{
  old_color_ = marker_->color();
  marker_->set_color(new_color_);
}

void MarkerChangeColorCommand::undo()
{
  marker_->set_color(old_color_);
}

MarkerChangeNameCommand::MarkerChangeNameCommand(TimelineMarker *marker, QString new_name) :
  marker_(marker),
  new_name_(new_name)
{
}

Project* MarkerChangeNameCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeNameCommand::redo()
{
  old_name_ = marker_->name();
  marker_->set_name(new_name_);
}

void MarkerChangeNameCommand::undo()
{
  marker_->set_name(old_name_);
}

MarkerChangeTimeCommand::MarkerChangeTimeCommand(TimelineMarker* marker, TimeRange time) :
  marker_(marker),
  new_time_(time)
{
}

Project* MarkerChangeTimeCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeTimeCommand::redo()
{
  old_time_ = marker_->time();
  marker_->set_time(new_time_);
}

void MarkerChangeTimeCommand::undo()
{
  marker_->set_time(old_time_);
}

}

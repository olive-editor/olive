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

#include "markerundo.h"

namespace olive {

MarkerAddCommand::MarkerAddCommand(Project *project, TimelineMarkerList *marker_list, const TimeRange &range, const QString &name, int color) :
    project_(project),
    marker_list_(marker_list),
    range_(range),
    name_(name),
    color_(color)
{
}

Project* MarkerAddCommand::GetRelevantProject() const 
{
  return project_;
}

void MarkerAddCommand::redo()
{
  added_marker_ = marker_list_->AddMarker(range_, name_, color_);
}

void MarkerAddCommand::undo()
{
  marker_list_->RemoveMarker(added_marker_);
}

MarkerRemoveCommand::MarkerRemoveCommand(Project *project, TimelineMarker *marker, TimelineMarkerList *marker_list) :
    project_(project),
    marker_(marker),
    marker_list_(marker_list),
    range_(marker->time()),
    name_(marker->name()),
    color_(marker->color())
{
}

Project* MarkerRemoveCommand::GetRelevantProject() const
{
  return project_;
}

void MarkerRemoveCommand::redo()
{
  marker_list_->RemoveMarker(marker_);
}

void MarkerRemoveCommand::undo()
{
  marker_list_->AddMarker(range_, name_, color_);
}

MarkerChangeColorCommand::MarkerChangeColorCommand(Project *project, TimelineMarker *marker, int new_color) :
    project_(project),
    marker_(marker),
    old_color_(marker->color()),
    new_color_(new_color)
{
}

Project* MarkerChangeColorCommand::GetRelevantProject() const
{
  return project_;
}

void MarkerChangeColorCommand::redo()
{
  marker_->set_color(new_color_);
}

void MarkerChangeColorCommand::undo()
{
  marker_->set_color(old_color_);
}

MarkerChangeNameCommand::MarkerChangeNameCommand(Project *project, TimelineMarker *marker, QString new_name) :
    project_(project),
    marker_(marker),
    old_name_(marker->name()),
    new_name_(new_name)
{
}

Project* MarkerChangeNameCommand::GetRelevantProject() const
{
  return project_;
}

void MarkerChangeNameCommand::redo()
{
  marker_->set_name(new_name_);
}

void MarkerChangeNameCommand::undo()
{
  marker_->set_name(old_name_);
}

MarkerChangeTimeCommand::MarkerChangeTimeCommand(Project* project, TimelineMarker* marker, TimeRange time) :
    project_(project),
    marker_(marker),
    old_time_(marker->time()),
    new_time_(time)
{
}

Project* MarkerChangeTimeCommand::GetRelevantProject() const
{
  return project_;
}

void MarkerChangeTimeCommand::redo()
{
  marker_->set_time(new_time_);
}

void MarkerChangeTimeCommand::undo()
{
  marker_->set_time(old_time_);
}

}

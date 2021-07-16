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

#ifndef TIMELINEUNDOWORKAREA_H
#define TIMELINEUNDOWORKAREA_H

#include "node/project/project.h"
#include "timeline/timelinepoints.h"

namespace olive {

class WorkareaSetEnabledCommand : public UndoCommand {
public:
  WorkareaSetEnabledCommand(Project *project, TimelinePoints* points, bool enabled) :
    project_(project),
    points_(points),
    old_enabled_(points_->workarea()->enabled()),
    new_enabled_(enabled)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

  virtual void redo() override
  {
    points_->workarea()->set_enabled(new_enabled_);
  }

  virtual void undo() override
  {
    points_->workarea()->set_enabled(old_enabled_);
  }

private:
  Project* project_;

  TimelinePoints* points_;

  bool old_enabled_;

  bool new_enabled_;

};

class WorkareaSetRangeCommand : public UndoCommand {
public:
  WorkareaSetRangeCommand(Project *project, TimelinePoints* points, const TimeRange& range) :
    project_(project),
    points_(points),
    old_range_(points_->workarea()->range()),
    new_range_(range)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

  virtual void redo() override
  {
    points_->workarea()->set_range(new_range_);
  }

  virtual void undo() override
  {
    points_->workarea()->set_range(old_range_);
  }

private:
  Project* project_;

  TimelinePoints* points_;

  TimeRange old_range_;

  TimeRange new_range_;

};

}

#endif // TIMELINEUNDOWORKAREA_H

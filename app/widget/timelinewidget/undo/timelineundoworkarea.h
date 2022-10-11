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

#ifndef TIMELINEUNDOWORKAREA_H
#define TIMELINEUNDOWORKAREA_H

#include "node/project/project.h"

namespace olive {

class WorkareaSetEnabledCommand : public UndoCommand {
public:
  WorkareaSetEnabledCommand(Project *project, TimelineWorkArea* points, bool enabled) :
    project_(project),
    points_(points),
    old_enabled_(points_->enabled()),
    new_enabled_(enabled)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

protected:
  virtual void redo() override
  {
    points_->set_enabled(new_enabled_);
  }

  virtual void undo() override
  {
    points_->set_enabled(old_enabled_);
  }

private:
  Project* project_;

  TimelineWorkArea* points_;

  bool old_enabled_;

  bool new_enabled_;

};

class WorkareaSetRangeCommand : public UndoCommand {
public:
  WorkareaSetRangeCommand(TimelineWorkArea *workarea, const TimeRange& range, const TimeRange &old_range) :
    workarea_(workarea),
    old_range_(old_range),
    new_range_(range)
  {
  }

  WorkareaSetRangeCommand(TimelineWorkArea *workarea, const TimeRange& range) :
    WorkareaSetRangeCommand(workarea, range, workarea->range())
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return Project::GetProjectFromObject(workarea_);
  }

protected:
  virtual void redo() override
  {
    workarea_->set_range(new_range_);
  }

  virtual void undo() override
  {
    workarea_->set_range(old_range_);
  }

private:
  TimelineWorkArea *workarea_;

  TimeRange old_range_;

  TimeRange new_range_;

};

}

#endif // TIMELINEUNDOWORKAREA_H

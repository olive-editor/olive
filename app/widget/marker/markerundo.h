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

#ifndef MARKERUNDO_H
#define MARKERUNDO_H

#include "undo/undocommand.h"
#include "timeline/timelinemarker.h"

namespace olive {

class MarkerAddCommand : public UndoCommand {
  public:
    MarkerAddCommand(Project* project, TimelineMarkerList* marker_list, const TimeRange& range, const QString& name,
                     int color = -1);

   virtual Project* GetRelevantProject() const override;

 protected:
   virtual void redo() override;
   virtual void undo() override;

 private:
   Project* project_;
   TimelineMarkerList* marker_list_;
   TimeRange range_;
   QString name_;
   int color_;

   TimelineMarker* added_marker_;
};

class MarkerRemoveCommand : public UndoCommand {
  public:
    MarkerRemoveCommand(Project* project, TimelineMarker* marker, TimelineMarkerList* marker_list);

    virtual Project* GetRelevantProject() const override;

  protected:
    virtual void redo() override;
    virtual void undo() override;

  private:
    Project* project_;
    TimelineMarker* marker_;
    TimelineMarkerList* marker_list_;
    TimeRange range_;
    QString name_;
    int color_;
};

class MarkerChangeColorCommand : public UndoCommand {
  public:
    MarkerChangeColorCommand(Project* project, TimelineMarker* marker, int new_color);

    virtual Project* GetRelevantProject() const override;

  protected:
    virtual void redo() override;
    virtual void undo() override;

  private:
    Project* project_;
    TimelineMarker* marker_;
    int old_color_;
    int new_color_;
};

}

#endif  // TIMELINEUNDOTRACK_H

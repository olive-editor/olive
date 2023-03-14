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

#ifndef IMPORTTIMELINETOOL_H
#define IMPORTTIMELINETOOL_H

#include "tool.h"

namespace olive {

class ImportTool : public TimelineTool
{
public:
  ImportTool(TimelineWidget* parent);

  virtual void DragEnter(TimelineViewMouseEvent *event) override;
  virtual void DragMove(TimelineViewMouseEvent *event) override;
  virtual void DragLeave(QDragLeaveEvent *event) override;
  virtual void DragDrop(TimelineViewMouseEvent *event) override;

  using DraggedFootageData = QVector<QPair<ViewerOutput*, QVector<Track::Reference> > >;

  void PlaceAt(const QVector<ViewerOutput *> &footage, const rational& start, bool insert, MultiUndoCommand *command, int track_offset = 0, bool jump_to_end = false);
  void PlaceAt(const DraggedFootageData &footage, const rational& start, bool insert, MultiUndoCommand *command, int track_offset = 0, bool jump_to_end = false);

  enum DropWithoutSequenceBehavior {
    kDWSAsk,
    kDWSAuto,
    kDWSManual,
    kDWSDisable
  };

private:
  void FootageToGhosts(rational ghost_start, const DraggedFootageData &footage, const rational &dest_tb, const int &track_start);

  void PrepGhosts(const rational &frame, const int &track_index);

  void DropGhosts(bool insert, MultiUndoCommand *parent_command);

  TimelineViewGhostItem* CreateGhost(const TimeRange &range, const rational &media_in, const Track::Reference &track);

  DraggedFootageData dragged_footage_;

  int import_pre_buffer_;

  rational ghost_offset_;

};

}

#endif // IMPORTTIMELINETOOL_H

/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

OLIVE_NAMESPACE_ENTER

class ImportTool : public TimelineTool
{
public:
  ImportTool(TimelineWidget* parent);

  virtual void DragEnter(TimelineViewMouseEvent *event) override;
  virtual void DragMove(TimelineViewMouseEvent *event) override;
  virtual void DragLeave(QDragLeaveEvent *event) override;
  virtual void DragDrop(TimelineViewMouseEvent *event) override;

  class DraggedFootage {
  public:
    DraggedFootage(Footage* f, quint64 streams) :
      footage_(f),
      streams_(streams)
    {
    }

    Footage* footage() const {
      return footage_;
    }

    const quint64& streams() const {
      return streams_;
    }

  private:
    Footage* footage_;

    quint64 streams_;

  };

  void PlaceAt(const QList<Footage*> &footage, const rational& start, bool insert);
  void PlaceAt(const QList<DraggedFootage> &footage, const rational& start, bool insert);

  enum DropWithoutSequenceBehavior {
    kDWSAsk,
    kDWSAuto,
    kDWSManual,
    kDWSDisable
  };

private:
  static DraggedFootage FootageToDraggedFootage(Footage* f);
  static QList<DraggedFootage> FootageToDraggedFootage(QList<Footage*> footage);

  void FootageToGhosts(rational ghost_start, const QList<DraggedFootage>& footage, const rational &dest_tb, const int &track_start);

  void PrepGhosts(const rational &frame, const int &track_index);

  void DropGhosts(bool insert);

  QList<DraggedFootage> dragged_footage_;

  int import_pre_buffer_;

};

OLIVE_NAMESPACE_EXIT

#endif // IMPORTTIMELINETOOL_H

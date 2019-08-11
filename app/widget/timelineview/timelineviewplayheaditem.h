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

#ifndef TIMELINEVIEWPLAYHEADITEM_H
#define TIMELINEVIEWPLAYHEADITEM_H

#include "timelineplayhead.h"
#include "timelineviewrect.h"

/**
 * @brief A graphical representation of the playhead on the Timeline
 */
class TimelineViewPlayheadItem : public TimelineViewRect
{
public:
  TimelineViewPlayheadItem(QGraphicsItem* parent = nullptr);

  void SetPlayhead(const int64_t& playhead);

  void SetTimebase(const rational& timebase);

  virtual void UpdateRect() override;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  int64_t playhead_;

  rational timebase_;

  TimelinePlayhead style_;
};

#endif // TIMELINEVIEWPLAYHEADITEM_H

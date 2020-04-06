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

#ifndef TIMELINEVIEWENDITEM_H
#define TIMELINEVIEWENDITEM_H

#include "timelineviewrect.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief An item placed at the end point of the Timeline to ensure the correct scene size
 */
class TimelineViewEndItem : public TimelineViewRect
{
public:
  TimelineViewEndItem(QGraphicsItem* parent = nullptr);

  void SetEndTime(const rational& time);

  void SetEndPadding(int padding);

  virtual void UpdateRect() override;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  rational end_time_;

  int end_padding_;
};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEVIEWENDITEM_H

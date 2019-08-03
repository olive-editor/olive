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

#ifndef TIMELINEVIEWGHOSTITEM_H
#define TIMELINEVIEWGHOSTITEM_H

#include "timelineviewrect.h"

class TimelineViewGhostItem : public TimelineViewRect
{
public:
  TimelineViewGhostItem(QGraphicsItem* parent = nullptr);

  void SetIn(const rational& in);
  void SetOut(const rational& out);

protected:
  virtual void UpdateRect() override;

//  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  rational in_;
  rational out_;
};

#endif // TIMELINEVIEWGHOSTITEM_H

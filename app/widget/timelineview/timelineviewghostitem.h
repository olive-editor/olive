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

#include <QVariant>

#include "project/item/footage/footage.h"
#include "timelineviewrect.h"

/**
 * @brief A graphical representation of changes the user is making before they apply it
 */
class TimelineViewGhostItem : public TimelineViewRect
{
public:
  TimelineViewGhostItem(QGraphicsItem* parent = nullptr);

  const rational& In();
  const rational& Out();

  rational Length();
  rational AdjustedLength();

  void SetIn(const rational& in);
  void SetOut(const rational& out);

  void SetInAdjustment(const rational& in_adj);
  void SetOutAdjustment(const rational& out_adj);

  rational GetAdjustedIn();
  rational GetAdjustedOut();

  const QVariant& data();
  void SetData(const QVariant& data);

  virtual void UpdateRect() override;

protected:

private:
  rational in_;
  rational out_;

  rational in_adj_;
  rational out_adj_;

  StreamPtr stream_;

  QVariant data_;
};

#endif // TIMELINEVIEWGHOSTITEM_H

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

#ifndef TIMELINEBLOCK_H
#define TIMELINEBLOCK_H

#include "node/block/block.h"
#include "panel/timeline/timeline.h"

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class TimelineBlock : public Block
{
  Q_OBJECT
public:
  TimelineBlock();

  virtual rational length() override;

  void AttachTimeline(TimelinePanel* timeline);

public slots:
  virtual void Process(const rational &time) override;

private:
  Block* current_block_;

  TimelinePanel* attached_timeline_;
};

#endif // TIMELINEBLOCK_H

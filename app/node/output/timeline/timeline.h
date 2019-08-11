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

#ifndef TIMELINEOUTPUT_H
#define TIMELINEOUTPUT_H

#include "node/block/block.h"
#include "panel/timeline/timeline.h"

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class TimelineOutput : public Block
{
  Q_OBJECT
public:
  TimelineOutput();

  virtual Type type() override;

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  void AttachTimeline(TimelinePanel* timeline);

  virtual rational length() override;

  virtual void Refresh() override;

public slots:
  virtual void Process(const rational &time) override;

private:
  QVector<Block*> block_cache_;

  Block* first_block();

  Block* attached_block();

  Block* first_block_;
  Block* current_block_;

  TimelinePanel* attached_timeline_;

private slots:
  void InsertBlock(Block* block, int index);
};

#endif // TIMELINEOUTPUT_H

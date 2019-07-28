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

#include "node/node.h"
#include "panel/timeline/timeline.h"

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class TimelineOutput : public Node
{
  Q_OBJECT
public:
  TimelineOutput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  void AttachTimeline(TimelinePanel* timeline);

  NodeInput* block_input();
  NodeOutput* texture_output();

public slots:
  virtual void Process(const rational &time) override;

private:
  Block* attached_block();
  Block* current_block_;

  NodeInput* block_input_;
  NodeOutput* texture_output_;

  TimelinePanel* attached_timeline_;
};

#endif // TIMELINEOUTPUT_H

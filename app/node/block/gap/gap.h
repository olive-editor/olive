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

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class GapBlock : public Block
{
public:
  GapBlock();

  virtual rational length() override;
  virtual void set_length(const rational &length) override;

private:
  rational length_;
};

#endif // TIMELINEBLOCK_H

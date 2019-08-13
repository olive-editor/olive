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

#ifndef GAPBLOCK_H
#define GAPBLOCK_H

#include "node/block/block.h"

/**
 * @brief Node that represents nothing in its respective track for a certain period of time
 */
class GapBlock : public Block
{
  Q_OBJECT
public:
  GapBlock();

  virtual Block * copy() override;

  virtual Type type() override;

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Description() override;

private:

};

#endif // TIMELINEBLOCK_H

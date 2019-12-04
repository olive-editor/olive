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

#include "gap.h"

GapBlock::GapBlock()
{
}

Node *GapBlock::copy() const
{
  GapBlock* c = new GapBlock();

  CopyParameters(this, c);

  return c;
}

Block::Type GapBlock::type() const
{
  return kGap;
}

QString GapBlock::Name() const
{
  return tr("Gap");
}

QString GapBlock::id() const
{
  return "org.olivevideoeditor.Olive.gap";
}

QString GapBlock::Description() const
{
  return tr("A time-based node that represents an empty space.");
}

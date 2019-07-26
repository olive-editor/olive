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

#ifndef BLOCK_H
#define BLOCK_H

#include "node/node.h"

class Block : public Node
{
  Q_OBJECT
public:
  Block();

  virtual QString Category() override;

  virtual rational in();
  virtual rational out();

  virtual rational length() = 0;

public slots:
//  virtual void Process(const rational &time) override;


};

#endif // BLOCK_H

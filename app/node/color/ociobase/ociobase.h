/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef OCIOBASE_H
#define OCIOBASE_H

#include "node/node.h"
#include "render/rendermanager.h"

namespace olive {

class OCIONodeBase : public Node
{
public:
  OCIONodeBase();

  NODE_DEFAULT_DESTRUCTOR(OCIONodeBase);

protected:
    Renderer* renderer() const
  {
    return attached_renderer_;
  }
  
  /**
   * @brief Renderer abstraction
   */
  Renderer* attached_renderer_;
};


} // olive

#endif // OCIOBASE_H

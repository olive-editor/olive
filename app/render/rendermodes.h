/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef RENDERMODE_H
#define RENDERMODE_H

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class RenderMode {
public:
  /**
   * @brief The primary different "modes" the renderer can function in
   */
  enum Mode {
    /**
     * This render is for realtime preview ONLY and does not need to be "perfect". Nodes can use lower-accuracy functions
     * to save performance when possible.
     */
    kOffline,

    /**
     * This render is some sort of export or master copy and Nodes should take time/bandwidth/system resources to produce
     * a higher accuracy version.
     */
    kOnline
  };
};

OLIVE_NAMESPACE_EXIT

#endif // RENDERMODE_H

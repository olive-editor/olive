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

#ifndef PROBESERVER_H
#define PROBESERVER_H

#include "project/item/footage/footage.h"

namespace olive {

/**
 * @brief Try to probe a Footage file by passing it through all available Decoders
 *
 * This is a helper function designed to abstract the process of communicating with several Decoders from the rest of
 * the application. This function will take a Footage file and manually pass it through the available Decoders' Probe()
 * functions until one indicates that it can decode this file. That Decoder will then dump information about the file
 * into the Footage object for use throughout the program.
 *
 * Probing may be a lengthy process and it's recommended to run this in a separate thread.
 *
 * @param f
 *
 * A Footage object with a valid filename. If the Footage does not have a valid filename (e.g. is empty or file doesn't
 * exist), this function will return FALSE.
 *
 * @return
 *
 * TRUE if a Decoder was successfully able to parse and probe this file. FALSE if not.
 */
bool ProbeMedia(Footage* f);

}

#endif // PROBESERVER_H

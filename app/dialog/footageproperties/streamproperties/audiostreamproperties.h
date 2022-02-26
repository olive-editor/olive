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

#ifndef AUDIOSTREAMPROPERTIES_H
#define AUDIOSTREAMPROPERTIES_H

#include "node/project/footage/footage.h"
#include "streamproperties.h"

namespace olive {

class AudioStreamProperties : public StreamProperties
{
public:
  AudioStreamProperties(Footage *footage, int audio_index);

  virtual void Accept(MultiUndoCommand* parent) override;

private:
  Footage *footage_;

  int audio_index_;

};

}

#endif // AUDIOSTREAMPROPERTIES_H

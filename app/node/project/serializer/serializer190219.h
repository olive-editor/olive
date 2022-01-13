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

#ifndef PROJECTSERIALIZER190219_H
#define PROJECTSERIALIZER190219_H

#include "serializer.h"

namespace olive {

class ProjectSerializer190219 : public ProjectSerializer
{
public:
  ProjectSerializer190219() = default;

protected:
  virtual LoadData Load(Project *project, QXmlStreamReader *reader, void *reserved) const override;

  virtual uint Version() const override
  {
    return 190219;
  }

};

}

#endif // SERIALIZER190219_H

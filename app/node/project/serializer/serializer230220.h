/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef PROJECTSERIALIZER230220_H
#define PROJECTSERIALIZER230220_H

#include "serializer.h"

namespace olive {

class ProjectSerializer230220 : public ProjectSerializer
{
public:
  ProjectSerializer230220() = default;

protected:
  virtual LoadData Load(Project *project, QXmlStreamReader *reader, LoadType load_type, void *reserved) const override;

  virtual void Save(QXmlStreamWriter *writer, const SaveData &data, void *reserved) const override;

  virtual uint Version() const override
  {
    return 230220;
  }

private:
  void PostConnect(const QVector<Node*> &nodes, SerializedData *project_data) const;

};

}

#endif // PROJECTSERIALIZER230220_H

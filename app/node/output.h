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

#ifndef NODEOUTPUT_H
#define NODEOUTPUT_H

#include "common/timerange.h"
#include "param.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A node parameter designed to serve data to the input of another node
 */
class NodeOutput : public NodeParam
{
  Q_OBJECT
public:
  /**
   * @brief NodeOutput Constructor
   */
  NodeOutput(const QString& id);

  /**
   * @brief Returns kOutput
   */
  virtual Type type() override;

  virtual QString name() override;

  virtual void Load(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled) override;

  virtual void Save(QXmlStreamWriter* writer) const override;

private:

};

OLIVE_NAMESPACE_EXIT

#endif // NODEOUTPUT_H

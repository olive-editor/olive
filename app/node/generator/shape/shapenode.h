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

#ifndef SHAPENODE_H
#define SHAPENODE_H

#include "shapenodebase.h"

namespace olive {

class ShapeNode : public ShapeNodeBase
{
  Q_OBJECT
public:
  ShapeNode();

  enum Type {
    kRectangle,
    kEllipse
  };

  NODE_DEFAULT_DESTRUCTOR(ShapeNode)
  NODE_COPY_FUNCTION(ShapeNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const QString& shader_id) const override;
  virtual NodeValueTable Value(const QString& output, NodeValueDatabase& value) const override;

  static QString kTypeInput;

private:


};

}

#endif // SHAPENODE_H

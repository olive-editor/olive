/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef MASKDISTORTNODE_H
#define MASKDISTORTNODE_H

#include "node/generator/polygon/polygon.h"

namespace olive {

class MaskDistortNode : public PolygonGenerator
{
  Q_OBJECT
public:
  MaskDistortNode();

  NODE_DEFAULT_FUNCTIONS(MaskDistortNode)

  virtual QString Name() const override
  {
    return tr("Mask");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.mask");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Apply a polygonal mask.");
  }

  virtual void Retranslate() override;

  virtual NodeValue Value(const ValueParams &p) const override;

  static const QString kInvertInput;
  static const QString kFeatherInput;

};

}

#endif // MASKDISTORTNODE_H

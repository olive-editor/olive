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

#ifndef MOSAICFILTERNODE_H
#define MOSAICFILTERNODE_H

#include "node/node.h"

namespace olive {

class MosaicFilterNode : public Node
{
  Q_OBJECT
public:
  MosaicFilterNode();

  NODE_DEFAULT_FUNCTIONS(MosaicFilterNode)

  virtual QString Name() const override
  {
    return tr("Mosaic");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.mosaicfilter");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryFilter};
  }

  virtual QString Description() const override
  {
    return tr("Apply a pixelated mosaic filter to video.");
  }

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;
  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

  static const QString kTextureInput;
  static const QString kHorizInput;
  static const QString kVertInput;

};

}

#endif // MOSAICFILTERNODE_H

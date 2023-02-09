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

#ifndef DROPSHADOWFILTER_H
#define DROPSHADOWFILTER_H

#include "node/node.h"

namespace olive {

class DropShadowFilter : public Node
{
  Q_OBJECT
public:
  DropShadowFilter();

  NODE_DEFAULT_FUNCTIONS(DropShadowFilter)

  virtual QString Name() const override { return tr("Drop Shadow"); }
  virtual QString id() const override { return QStringLiteral("org.olivevideoeditor.Olive.dropshadow"); }
  virtual QVector<CategoryID> Category() const override { return {kCategoryFilter}; }
  virtual QString Description() const override { return tr("Adds a drop shadow to an image."); }

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;
  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static const QString kTextureInput;
  static const QString kColorInput;
  static const QString kDistanceInput;
  static const QString kAngleInput;
  static const QString kSoftnessInput;
  static const QString kOpacityInput;
  static const QString kFastInput;

};

}

#endif // DROPSHADOWFILTER_H

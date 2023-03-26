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

#ifndef CROPDISTORTNODE_H
#define CROPDISTORTNODE_H

#include <QVector2D>

#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {

class CropDistortNode : public Node
{
  Q_OBJECT
public:
  CropDistortNode();

  NODE_DEFAULT_FUNCTIONS(CropDistortNode)

  virtual QString Name() const override
  {
    return tr("Crop");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.crop");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Crop the edges of an image.");
  }

  virtual void Retranslate() override;

  virtual NodeValue Value(const ValueParams &p) const override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

  virtual void UpdateGizmoPositions(const ValueParams &p) override;

  static const QString kTextureInput;
  static const QString kLeftInput;
  static const QString kTopInput;
  static const QString kRightInput;
  static const QString kBottomInput;
  static const QString kFeatherInput;

protected slots:
  virtual void GizmoDragMove(double delta_x, double delta_y, const Qt::KeyboardModifiers &modifiers) override;

private:
  void CreateCropSideInput(const QString& id);

  // Gizmo variables
  PointGizmo *point_gizmo_[kGizmoScaleCount];
  PolygonGizmo *poly_gizmo_;
  QVector2D temp_resolution_;

};

}

#endif // CROPDISTORTNODE_H

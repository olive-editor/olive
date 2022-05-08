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

#ifndef CORNERPINDISTORTNODE_H
#define CORNERPINDISTORTNODE_H

#include <QVector2D>

#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {
class CornerPinDistortNode : public Node
{
  Q_OBJECT
public:
  CornerPinDistortNode();

  NODE_DEFAULT_FUNCTIONS(CornerPinDistortNode)

  virtual QString Name() const override
  {
    return tr("Corner Pin");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.cornerpin");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryDistort};
  }

  virtual QString Description() const override
  {
    return tr("Distort the image by dragging the corners.");
  }

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

  virtual void UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals) override;

  /**
   * @brief Convenience function - converts the 2D slider values from being
   * an offset to the actual pixel value.
   */
  QPointF ValueToPixel(int value, const NodeValueRow &row, const QVector2D &resolution) const;

  static const QString kTextureInput;
  static const QString kPerspectiveInput;
  static const QString kTopLeftInput;
  static const QString kTopRightInput;
  static const QString kBottomRightInput;
  static const QString kBottomLeftInput;

protected slots:
  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

private:
  // Gizmo variables
  static const int kGizmoCornerCount = 4;
  PointGizmo *gizmo_resize_handle_[kGizmoCornerCount];
  PolygonGizmo *gizmo_whole_rect_;

};

}


#endif // CORNERPINDISTORTNODE_H

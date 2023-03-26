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

#ifndef SHAPENODEBASE_H
#define SHAPENODEBASE_H

#include "generatorwithmerge.h"
#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/inputdragger.h"
#include "node/node.h"

namespace olive {

class ShapeNodeBase : public GeneratorWithMerge
{
  Q_OBJECT
public:
  ShapeNodeBase(bool create_color_input = true);

  virtual void Retranslate() override;

  virtual void UpdateGizmoPositions(const ValueParams &p) override;

  void SetRect(QRectF rect, const VideoParams &sequence_res, MultiUndoCommand *command);

  static const QString kPositionInput;
  static const QString kSizeInput;
  static const QString kColorInput;

protected:
  PolygonGizmo *poly_gizmo() const
  {
    return poly_gizmo_;
  }

protected slots:
  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

private:
  QVector2D GenerateGizmoAnchor(const QVector2D &pos, const QVector2D &size, NodeGizmo *gizmo, QVector2D *pt = nullptr) const;

  bool IsGizmoTop(NodeGizmo *g) const;
  bool IsGizmoBottom(NodeGizmo *g) const;
  bool IsGizmoLeft(NodeGizmo *g) const;
  bool IsGizmoRight(NodeGizmo *g) const;
  bool IsGizmoHorizontalCenter(NodeGizmo *g) const;
  bool IsGizmoVerticalCenter(NodeGizmo *g) const;
  bool IsGizmoCorner(NodeGizmo *g) const;

  // Gizmo variables
  static const int kGizmoWholeRect = kGizmoScaleCount;
  PointGizmo *point_gizmo_[kGizmoScaleCount];
  PolygonGizmo *poly_gizmo_;

};

}

#endif // SHAPENODEBASE_H

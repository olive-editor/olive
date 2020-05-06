/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef POLYGONGENERATOR_H
#define POLYGONGENERATOR_H

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class PolygonGenerator : public Node
{
public:
  PolygonGenerator();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QList<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const override;
  virtual QString ShaderFragmentCode(const NodeValueDatabase&) const override;

  virtual bool HasGizmos() const override;
  virtual void DrawGizmos(NodeValueDatabase& db, QPainter *p, const QVector2D &scale) const override;

  /*
  virtual bool GizmoPress(const QPointF &p) override;
  virtual void GizmoMove(const QPointF &p) override;
  virtual void GizmoRelease(const QPointF &p) override;
  */

private:
  NodeInputArray* points_input_;

  NodeInput* color_input_;

};

OLIVE_NAMESPACE_EXIT

#endif // POLYGONGENERATOR_H

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

#ifndef POLYGONGENERATOR_H
#define POLYGONGENERATOR_H

#include <QPainterPath>

#include "common/bezier.h"
#include "node/generator/shape/generatorwithmerge.h"
#include "node/gizmo/line.h"
#include "node/gizmo/path.h"
#include "node/gizmo/point.h"
#include "node/node.h"
#include "node/inputdragger.h"

namespace olive {

class PolygonGenerator : public GeneratorWithMerge
{
  Q_OBJECT
public:
  PolygonGenerator();

  NODE_DEFAULT_FUNCTIONS(PolygonGenerator)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void GenerateFrame(FramePtr frame, const GenerateJob &job) const override;

  virtual void UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals) override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

  static const QString kPointsInput;
  static const QString kColorInput;

protected:
  ShaderJob GetGenerateJob(const NodeValueRow &value, const VideoParams &params) const;

protected slots:
  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

private:
  static void AddPointToPath(QPainterPath *path, const Bezier &before, const Bezier &after);

  static QPainterPath GeneratePath(const NodeValueArray &points, int size);

  template<typename T>
  void ValidateGizmoVectorSize(QVector<T*> &vec, int new_sz);

  template<typename T>
  NodeGizmo *CreateAppropriateGizmo();

  PathGizmo *poly_gizmo_;
  QVector<PointGizmo*> gizmo_position_handles_;
  QVector<PointGizmo*> gizmo_bezier_handles_;
  QVector<LineGizmo*> gizmo_bezier_lines_;

};

}

#endif // POLYGONGENERATOR_H

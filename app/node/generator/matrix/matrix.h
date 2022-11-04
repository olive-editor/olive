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

#ifndef MATRIXGENERATOR_H
#define MATRIXGENERATOR_H

#include <QVector2D>

#include "node/node.h"
#include "node/inputdragger.h"

namespace olive {

class MatrixGenerator : public Node
{
  Q_OBJECT
public:
  MatrixGenerator();

  NODE_DEFAULT_FUNCTIONS(MatrixGenerator)

  virtual QString Name() const override;
  virtual QString ShortName() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static const QString kPositionInput;
  static const QString kRotationInput;
  static const QString kScaleInput;
  static const QString kUniformScaleInput;
  static const QString kAnchorInput;

protected:
  QMatrix4x4 GenerateMatrix(const NodeValueRow &value, bool ignore_anchor, bool ignore_position, bool ignore_scale, const QMatrix4x4 &mat, bool is_gizmo = false) const;
  static QMatrix4x4 GenerateMatrix(const QVector2D &pos,
                                   const float &rot,
                                   const QVector2D &scale,
                                   bool uniform_scale,
                                   const QVector2D &anchor,
                                   QMatrix4x4 mat);

  virtual void InputValueChangedEvent(const QString& input, int element) override;

};

}

#endif // TRANSFORMDISTORT_H

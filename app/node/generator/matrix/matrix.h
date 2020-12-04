/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString ShortName() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual NodeValueTable Value(NodeValueDatabase& value) const override;

protected:
  QMatrix4x4 GenerateMatrix(NodeValueDatabase &value, bool take, bool ignore_anchor, bool ignore_position, bool ignore_scale) const;
  static QMatrix4x4 GenerateMatrix(const QVector2D &pos,
                                   const float &rot,
                                   const QVector2D &scale,
                                   bool uniform_scale,
                                   const QVector2D &anchor);

  NodeInput* position_input() const
  {
    return position_input_;
  }

  NodeInput* rotation_input() const
  {
    return rotation_input_;
  }

  NodeInput* scale_input() const
  {
    return scale_input_;
  }

  NodeInput* uniform_scale_input() const
  {
    return uniform_scale_input_;
  }

  NodeInput* anchor_input() const
  {
    return anchor_input_;
  }

private:
  NodeInput* position_input_;

  NodeInput* rotation_input_;

  NodeInput* scale_input_;

  NodeInput* uniform_scale_input_;

  NodeInput* anchor_input_;

private slots:
  void UniformScaleChanged();

};

}

#endif // TRANSFORMDISTORT_H

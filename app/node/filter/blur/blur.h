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

#ifndef BLURFILTERNODE_H
#define BLURFILTERNODE_H

#include "node/gizmo/point.h"
#include "node/node.h"

namespace olive {

class BlurFilterNode : public Node
{
  Q_OBJECT
public:
  BlurFilterNode();

  enum Method {
    kBox,
    kGaussian,
    kDirectional,
    kRadial
  };

  NODE_DEFAULT_FUNCTIONS(BlurFilterNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;
  virtual NodeValue Value(const ValueParams &globals) const override;

  Method GetMethod() const
  {
    return static_cast<Method>(GetStandardValue(kMethodInput).toInt());
  }

  virtual void UpdateGizmoPositions(const ValueParams &p) override;

  static const QString kTextureInput;
  static const QString kMethodInput;
  static const QString kRadiusInput;
  static const QString kHorizInput;
  static const QString kVertInput;
  static const QString kRepeatEdgePixelsInput;

  static const QString kDirectionalDegreesInput;

  static const QString kRadialCenterInput;

protected slots:
  virtual void GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers) override;

protected:
  virtual void InputValueChangedEvent(const QString& input, int element) override;

private:
  void UpdateInputs(Method method);

  PointGizmo *radial_center_gizmo_;

};

}

#endif // BLURFILTERNODE_H

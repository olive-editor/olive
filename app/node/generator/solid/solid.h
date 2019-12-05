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

#ifndef SOLIDGENERATOR_H
#define SOLIDGENERATOR_H

#include <QOpenGLTexture>

#include "node/node.h"

/**
 * @brief A node that generates a solid color
 */
class SolidGenerator : public Node
{
  Q_OBJECT
public:
  SolidGenerator();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual QString Code() const override;

private:
  NodeInput* color_input_;

  QOpenGLTexture* texture_;
};

#endif // SOLIDGENERATOR_H

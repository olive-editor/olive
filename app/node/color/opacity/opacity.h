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

#ifndef OPACITYNODE_H
#define OPACITYNODE_H

#include "node/node.h"
#include "render/gl/shaderptr.h"

class OpacityNode : public Node
{
  Q_OBJECT
public:
  OpacityNode();

  virtual QString Name() override;
  virtual QString Category() override;
  virtual QString Description() override;

  virtual QString id() override;

  virtual QVariant Value(NodeOutput *output, const rational &time) override;

  NodeInput* texture_input();

  NodeOutput* texture_output();

private:
  NodeInput* opacity_input_;

  NodeInput* texture_input_;

  NodeOutput* texture_output_;

};

#endif // OPACITYNODE_H

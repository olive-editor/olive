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

#ifndef TRANSFORMDISTORT_H
#define TRANSFORMDISTORT_H

#include "node/node.h"

class TransformDistort : public Node
{
  Q_OBJECT
public:
  TransformDistort();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  NodeOutput* matrix_output();

protected:
  virtual QVariant Value(NodeOutput *output, const rational &time) override;

private:
  NodeInput* position_input_;

  NodeInput* rotation_input_;

  NodeInput* scale_input_;

  NodeInput* anchor_input_;

  NodeOutput* matrix_output_;

};

#endif // TRANSFORMDISTORT_H

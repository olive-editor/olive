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

#ifndef NODEINPUT_H
#define NODEINPUT_H

#include "keyframe.h"
#include "param.h"

class NodeInput : public NodeParam
{
public:
  NodeInput();

  virtual Type type() override;

  void add_data_input(const DataType& data_type);

  bool can_accept_type(const DataType& data_type);

  bool can_accept_multiple_inputs();
  void set_can_accept_multiple_inputs(bool b);

  QVariant get_value(const rational &time);

  bool keyframing();
  void set_keyframing(bool k);

  const QList<DataType>& inputs();

private:
  QList<DataType> inputs_;

  QList<NodeKeyframe> keyframes_;

  bool keyframing_;

  bool can_accept_multiple_inputs_;
};

#endif // NODEINPUT_H

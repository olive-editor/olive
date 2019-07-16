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

#ifndef NODEOUTPUT_H
#define NODEOUTPUT_H

#include "param.h"

class NodeOutput : public NodeParam
{
public:
  NodeOutput();

  virtual Type type() override;

  const DataType& data_type();
  void set_data_type(const DataType& type);

  virtual const QVariant& get_value(const rational &time);
  virtual void set_value(const QVariant& value);

private:
  DataType data_type_;

  QVariant value_;
};

#endif // NODEOUTPUT_H

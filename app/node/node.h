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

#ifndef NODE_H
#define NODE_H

#include <QObject>

#include "node/input.h"
#include "node/output.h"
#include "rational.h"

class Node : public QObject
{
  Q_OBJECT
public:
  Node(QObject* parent = nullptr);

  virtual QString Name() = 0;
  virtual QString Category();
  virtual QString Description();

  virtual void InvalidateCache(const rational& start_range, const rational& end_range);

  using ParamList = QList<NodeParam *>;

  ParamList Parameters();

public slots:
  virtual void Process(const rational& time) = 0;
};

#endif // NODE_H

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

#ifndef NODEGIZMO_H
#define NODEGIZMO_H

#include <QObject>
#include <QPainter>

#include "node/globals.h"

namespace olive {

class NodeGizmo : public QObject
{
  Q_OBJECT
public:
  explicit NodeGizmo(QObject *parent = nullptr);
  virtual ~NodeGizmo() override;

  virtual void Draw(QPainter *p) const {}

  const ValueParams &GetGlobals() const { return globals_; }
  void SetGlobals(const ValueParams &globals) { globals_ = globals; }

  bool IsVisible() const { return visible_; }
  void SetVisible(bool e) { visible_ = e; }

signals:

private:
  ValueParams globals_;

  bool visible_;

};

}

#endif // NODEGIZMO_H

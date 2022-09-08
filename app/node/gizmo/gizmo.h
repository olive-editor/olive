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

  const NodeGlobals &GetGlobals() const { return globals_; }
  void SetGlobals(const NodeGlobals &globals) { globals_ = globals; }

  bool IsVisible() const { return visible_; }
  void SetVisible(bool e) { visible_ = e; }

  void SetSelected( bool s) {
    selected_ = s;
  }

  bool IsSelected() const {
    return selected_;
  }

  void SetHovered( bool h) {
    hovered_ = h;
  }

  bool IsHovered() const {
    return hovered_;
  }

  // This function can be overriden for gizmo that are moved in group,
  // for example the control point of a bezier gizmo
  virtual bool CanBeDraggedInGroup() const {
    return true;
  }

signals:

private:
  NodeGlobals globals_;

  bool visible_;
  bool selected_;
  bool hovered_;
};

}

#endif // NODEGIZMO_H

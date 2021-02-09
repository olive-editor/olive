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

#ifndef NODEINPUTDRAGGER_H
#define NODEINPUTDRAGGER_H

#include "common/rational.h"
#include "node/keyframe.h"
#include "node/param.h"

namespace olive {

class NodeInputDragger
{
public:
  NodeInputDragger();

  bool IsStarted() const;

  void Start(const NodeKeyframeTrackReference& input, const rational& time);

  void Drag(QVariant value);

  void End();

private:
  NodeKeyframeTrackReference input_;

  rational time_;

  QVariant start_value_;

  QVariant end_value_;

  NodeKeyframe* dragging_key_;

  bool drag_created_key_;

};

}

#endif // NODEINPUTDRAGGER_H

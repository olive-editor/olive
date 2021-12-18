/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "keyframeviewinputconnection.h"

#include "keyframeview.h"

namespace olive {

KeyframeViewInputConnection::KeyframeViewInputConnection(const NodeKeyframeTrackReference &input, KeyframeView *parent) :
  QObject(parent),
  keyframe_view_(parent),
  input_(input),
  y_(0),
  y_behavior_(kSingleRow),
  brush_(Qt::white)
{
  Node *n = input.input().node();

  connect(n, &Node::KeyframeAdded, this, &KeyframeViewInputConnection::AddKeyframe);
  connect(n, &Node::KeyframeRemoved, this, &KeyframeViewInputConnection::RemoveKeyframe);
  connect(n, &Node::KeyframeTimeChanged, this, &KeyframeViewInputConnection::KeyframeChanged);
  connect(n, &Node::KeyframeTypeChanged, this, &KeyframeViewInputConnection::KeyframeChanged);
  connect(n, &Node::KeyframeTypeChanged, this, &KeyframeViewInputConnection::KeyframeTypeChanged);
  connect(n, &Node::KeyframeValueChanged, this, &KeyframeViewInputConnection::KeyframeChanged);
}

void KeyframeViewInputConnection::SetKeyframeY(int y)
{
  if (y_ != y) {
    y_ = y;

    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::SetYBehavior(YBehavior e)
{
  if (y_behavior_ != e) {
    y_behavior_ = e;

    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::SetBrush(const QBrush &brush)
{
  if (brush_ != brush) {
    brush_ = brush;

    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::AddKeyframe(NodeKeyframe *key)
{
  if (key->key_track_ref() == input_) {
    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::RemoveKeyframe(NodeKeyframe *key)
{
  if (key->key_track_ref() == input_) {
    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::KeyframeChanged(NodeKeyframe *key)
{
  if (key->key_track_ref() == input_) {
    emit RequireUpdate();
  }
}

void KeyframeViewInputConnection::KeyframeTypeChanged(NodeKeyframe *key)
{
  if (key->key_track_ref() == input_) {
    emit TypeChanged();
  }
}

}

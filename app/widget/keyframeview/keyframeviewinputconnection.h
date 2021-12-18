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

#ifndef KEYFRAMEVIEWINPUTCONNECTION_H
#define KEYFRAMEVIEWINPUTCONNECTION_H

#include <QObject>

#include "node/node.h"
#include "node/param.h"

namespace olive {

class KeyframeView;

class KeyframeViewInputConnection : public QObject
{
  Q_OBJECT
public:
  KeyframeViewInputConnection(const NodeKeyframeTrackReference &input, KeyframeView *parent);

  const int &GetKeyframeY() const
  {
    return y_;
  }

  void SetKeyframeY(int y);

  enum YBehavior {
    kSingleRow,
    kValueIsHeight
  };

  void SetYBehavior(YBehavior e);

  const QVector<NodeKeyframe*> GetKeyframes() const
  {
    return input_.input().node()->GetKeyframeTracks(input_.input()).at(input_.track());
  }

  const QBrush &GetBrush() const
  {
    return brush_;
  }

  void SetBrush(const QBrush &brush);

signals:
  void RequireUpdate();

  void TypeChanged();

private:
  KeyframeView *keyframe_view_;

  NodeKeyframeTrackReference input_;

  int y_;

  YBehavior y_behavior_;

  QBrush brush_;

private slots:
  void AddKeyframe(NodeKeyframe *key);

  void RemoveKeyframe(NodeKeyframe *key);

  void KeyframeChanged(NodeKeyframe *key);

  void KeyframeTypeChanged(NodeKeyframe *key);

};

}

#endif // KEYFRAMEVIEWINPUTCONNECTION_H

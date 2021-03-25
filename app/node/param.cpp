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

#include "param.h"

#include "node.h"

namespace olive {

QString NodeInput::name() const
{
  if (IsValid()) {
    return node_->GetInputName(input_);
  } else {
    return QString();
  }
}

bool NodeInput::IsConnected() const
{
  if (IsValid()) {
    return node_->IsInputConnected(*this);
  } else {
    return false;
  }
}

bool NodeInput::IsKeyframing() const
{
  if (IsValid()) {
    return node_->IsInputKeyframing(*this);
  } else {
    return false;
  }
}

bool NodeInput::IsArray() const
{
  if (IsValid()) {
    return node_->InputIsArray(input_);
  } else {
    return false;
  }
}

NodeOutput NodeInput::GetConnectedOutput() const
{
  if (IsValid()) {
    return node_->GetConnectedOutput(*this);
  } else {
    return NodeOutput();
  }
}

NodeValue::Type NodeInput::GetDataType() const
{
  if (IsValid()) {
    return node_->GetInputDataType(input_);
  } else {
    return NodeValue::kNone;
  }
}

QStringList NodeInput::GetComboBoxStrings() const
{
  if (IsValid()) {
    return node_->GetComboBoxStrings(input_);
  } else {
    return QStringList();
  }
}

QVariant NodeInput::GetProperty(const QString &key) const
{
  if (IsValid()) {
    return node_->GetInputProperty(input_, key);
  } else {
    return QVariant();
  }
}

QVariant NodeInput::GetValueAtTime(const rational &time) const
{
  if (IsValid()) {
    return node_->GetValueAtTime(*this, time);
  } else {
    return QVariant();
  }
}

NodeKeyframe* NodeInput::GetKeyframeAtTimeOnTrack(const rational &time, int track) const
{
  if (IsValid()) {
    return node_->GetKeyframeAtTimeOnTrack(*this, time, track);
  } else {
    return nullptr;
  }
}

QVariant NodeInput::GetSplitDefaultValueForTrack(int track) const
{
  if (IsValid()) {
    return node_->GetSplitDefaultValueOnTrack(input_, track);
  } else {
    return QVariant();
  }
}

uint qHash(const NodeInput &i)
{
  return qHash(i.node()) ^ qHash(i.input()) ^ qHash(i.element());
}

NodeOutput::NodeOutput(Node *n)
{
  node_ = n;
  output_ = Node::kDefaultOutput;
}

uint qHash(const NodeKeyframeTrackReference &i)
{
  return qHash(i.input()) & qHash(i.track());
}

uint qHash(const NodeInputPair &i)
{
  return qHash(i.node) & qHash(i.input);
}

}

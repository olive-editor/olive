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

#ifndef NODEPARAM_H
#define NODEPARAM_H

#include <QString>

#include "common/rational.h"
#include "value.h"

namespace olive {

class Node;
class NodeKeyframe;

enum InputFlag {
  /// By default, inputs are keyframable, connectable, and NOT arrays
  kInputFlagNormal = 0x0,
  kInputFlagArray = 0x1,
  kInputFlagNotKeyframable = 0x2,
  kInputFlagNotConnectable = 0x4,
  kInputFlagHidden = 0x8
};

class InputFlags {
public:
  explicit InputFlags()
  {
    f_ = kInputFlagNormal;
  }

  explicit InputFlags(uint64_t flags)
  {
    f_ = flags;
  }

  bool operator&(const InputFlag& f) const
  {
    return f_ & f;
  }

private:
  uint64_t f_;

};

struct NodeInputPair {
  bool operator==(const NodeInputPair& rhs) const
  {
    return node == rhs.node && input == rhs.input;
  }

  Node* node;
  QString input;
};

/**
 * @brief Defines a Node input
 */
class NodeInput
{
public:
  NodeInput()
  {
    node_ = nullptr;
    element_ = -1;
  }

  NodeInput(Node* n, const QString& i, int e = -1)
  {
    node_ = n;
    input_ = i;
    element_ = e;
  }

  bool operator==(const NodeInput& rhs) const
  {
    return node_ == rhs.node_ && input_ == rhs.input_ && element_ == rhs.element_;
  }

  bool operator!=(const NodeInput& rhs) const
  {
    return !(*this == rhs);
  }

  bool operator<(const NodeInput& rhs) const
  {
    if (node_ != rhs.node_) {
      return node_ < rhs.node_;
    }

    if (input_ != rhs.input_) {
      return input_ < rhs.input_;
    }

    return element_ < rhs.element_;
  }

  Node* node() const
  {
    return node_;
  }

  NodeInputPair input_pair() const
  {
    return {node_, input_};
  }

  const QString& input() const
  {
    return input_;
  }

  const int &element() const
  {
    return element_;
  }

  void set_node(Node *node)
  {
    node_ = node;
  }

  void set_input(const QString &input)
  {
    input_ = input;
  }

  void set_element(int e)
  {
    element_ = e;
  }

  QString name() const;

  bool IsValid() const
  {
    return node_ && !input_.isEmpty() && element_ >= -1;
  }

  bool IsHidden() const;

  bool IsConnected() const;

  bool IsKeyframing() const;

  bool IsArray() const;

  InputFlags GetFlags() const;

  Node *GetConnectedOutput() const;

  NodeValue::Type GetDataType() const;

  QVariant GetDefaultValue() const;

  QStringList GetComboBoxStrings() const;

  QVariant GetProperty(const QString& key) const;

  QVariant GetValueAtTime(const rational& time) const;

  NodeKeyframe *GetKeyframeAtTimeOnTrack(const rational& time, int track) const;

  QVariant GetSplitDefaultValueForTrack(int track) const;

  int GetArraySize() const;

  void Reset()
  {
    *this = NodeInput();
  }

private:
  Node* node_;
  QString input_;
  int element_;

};

struct InputElementPair {
  QString input;
  int element;

  bool operator<(const InputElementPair &rhs) const
  {
    if (input != rhs.input) {
      return input < rhs.input;
    }

    return element < rhs.element;
  }

  bool operator==(const InputElementPair &rhs) const
  {
    return input == rhs.input && element == rhs.element;
  }

  bool operator!=(const InputElementPair &rhs) const
  {
    return !(*this == rhs);
  }
};

class NodeKeyframeTrackReference {
public:
  NodeKeyframeTrackReference()
  {
    track_ = -1;
  }

  NodeKeyframeTrackReference(const NodeInput& input, int track = 0)
  {
    input_ = input;
    track_ = track;
  }

  bool operator==(const NodeKeyframeTrackReference& rhs) const
  {
    return input_ == rhs.input_ && track_ == rhs.track_;
  }

  const NodeInput& input() const
  {
    return input_;
  }

  int track() const
  {
    return track_;
  }

  bool IsValid() const
  {
    return input_.IsValid() && track_ >= 0;
  }

  void Reset()
  {
    *this = NodeKeyframeTrackReference();
  }

private:
  NodeInput input_;
  int track_;

};

uint qHash(const NodeInputPair& i);
uint qHash(const NodeInput& i);
uint qHash(const NodeKeyframeTrackReference& i);

}

Q_DECLARE_METATYPE(olive::NodeInput)
Q_DECLARE_METATYPE(olive::NodeKeyframeTrackReference)

#endif // NODEPARAM_H

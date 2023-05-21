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

#ifndef NODEPARAM_H
#define NODEPARAM_H

#include <QString>

#include "value.h"

namespace olive {

class Node;
class NodeKeyframe;

class InputFlag
{
public:
  explicit InputFlag(uint64_t f = 0) noexcept { f_ = f; }

  InputFlag &operator|=(const InputFlag &i)
  {
    f_ |= i.f_;
    return *this;
  };

  InputFlag operator|(const InputFlag &rhs) const
  {
    InputFlag f = *this;
    f.f_ |= rhs.f_;
    return f;
  };

  InputFlag &operator&=(const InputFlag &i)
  {
    f_ &= i.f_;
    return *this;
  }

  InputFlag operator&(const InputFlag &rhs) const
  {
    InputFlag f = *this;
    f.f_ &= rhs.f_;
    return f;
  }

  operator bool() const { return f_; }
  bool operator!() const { return !f_; }

  InputFlag operator~() const
  {
    return InputFlag(~f_);
  }

  const uint64_t &value() const { return f_; }

private:
  uint64_t f_;

};

static const InputFlag kInputFlagNormal = InputFlag(0x0);
static const InputFlag kInputFlagArray = InputFlag(0x1);
static const InputFlag kInputFlagNotKeyframable = InputFlag(0x2);
static const InputFlag kInputFlagNotConnectable = InputFlag(0x4);
static const InputFlag kInputFlagHidden = InputFlag(0x8);
static const InputFlag kInputFlagIgnoreInvalidations = InputFlag(0x10);
static const InputFlag kInputFlagDontAutoConvert = InputFlag(0x20);
static const InputFlag kInputFlagStatic = kInputFlagNotKeyframable | kInputFlagNotConnectable;

struct NodeInputPair
{
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

  InputFlag GetFlags() const;

  QString GetInputName() const;

  Node *GetConnectedOutput() const;

  type_t GetDataType() const;

  value_t GetDefaultValue() const;

  size_t GetChannelCount() const;

  QStringList GetComboBoxStrings() const;

  value_t GetProperty(const QString& key) const;
  QHash<QString, value_t> GetProperties() const;

  value_t GetValueAtTime(const rational& time) const;

  NodeKeyframe *GetKeyframeAtTimeOnTrack(const rational& time, int track) const;

  value_t::component_t GetSplitDefaultValueForTrack(int track) const;

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

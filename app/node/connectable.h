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

#ifndef CONNECTABLE_H
#define CONNECTABLE_H

#include <QMap>
#include <QObject>
#include <QVector>

namespace olive {

class Node;
class NodeInput;

class NodeConnectable : public QObject
{
  Q_OBJECT
public:
  NodeConnectable() = default;

  struct InputConnection {
    InputConnection(NodeInput* i = nullptr, int e = -1)
    {
      input = i;
      element = e;
    }

    bool operator==(const InputConnection& rhs) const
    {
      return input == rhs.input && element == rhs.element;
    }

    NodeInput* input;
    int element;
  };

  static void ConnectEdge(Node* output, NodeInput* input, int element = -1);

  static void DisconnectEdge(Node* output, NodeInput* input, int element = -1);

  struct Edge {
    Node* output;
    NodeInput* input;
    int element;
  };

protected:
  const std::vector<InputConnection>& output_connections() const
  {
    return output_connections_;
  }

  const std::map<int, Node*>& input_connections() const
  {
    return input_connections_;
  }

private:
  std::vector<InputConnection> output_connections_;

  std::map<int, Node*> input_connections_;

};

uint qHash(const NodeConnectable::InputConnection& r, uint seed = 0);

}

#endif // CONNECTABLE_H

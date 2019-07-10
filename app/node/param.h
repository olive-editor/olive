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

#ifndef NODEPARAM_H
#define NODEPARAM_H

#include <QObject>

#include "node/edge.h"

class Node;

class NodeParam : public QObject
{
public:
  enum Type {
    kInput,
    kOutput,
    kBidirectional
  };

  enum DataType {
    kNone,
    kInt,
    kFloat,
    kColor,
    kString,
    kBoolean,
    kFont,
    kFile,
    kTexture,
    kMatrix,
    kBlock,
    kAny
  };

  NodeParam(Node* parent);

  virtual Type type() = 0;

  const QString& name();
  void set_name(const QString& name);

  static bool AreDataTypesCompatible(const DataType& output_type, const DataType& input_type);
  static bool AreDataTypesCompatible(const DataType& output_type, const QList<DataType>& input_types);

  static void ConnectEdge(NodeOutput *output, NodeInput *input);
  static void DisconnectEdge(NodeEdgePtr edge);

  static QString GetDefaultDataTypeName(const DataType &type);

private:
  QVector<NodeEdgePtr> edges_;

  QString name_;
};

#endif // NODEPARAM_H

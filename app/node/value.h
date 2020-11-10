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

#ifndef VALUE_H
#define VALUE_H

#include <QString>

#include "input.h"
#include "render/shadervalue.h"

OLIVE_NAMESPACE_ENTER

class NodeValue
{
public:
  NodeValue();
  NodeValue(const NodeParam::DataType& type, const QVariant& data, const Node* from, const QString& tag = QString());

  const NodeParam::DataType& type() const
  {
    return type_;
  }

  const QVariant& data() const
  {
    return data_;
  }

  const QString& tag() const
  {
    return tag_;
  }

  const Node* source() const
  {
    return from_;
  }

  bool operator==(const NodeValue& rhs) const;

private:
  NodeParam::DataType type_;
  QVariant data_;
  const Node* from_;
  QString tag_;

};

class NodeValueTable
{
public:
  NodeValueTable() = default;

  QVariant Get(const NodeParam::DataType& type, const QString& tag = QString()) const;
  NodeValue GetWithMeta(const NodeParam::DataType& type, const QString& tag = QString()) const;
  QVariant Take(const NodeParam::DataType& type, const QString& tag = QString());
  NodeValue TakeWithMeta(const NodeParam::DataType& type, const QString& tag = QString());

  void Push(const NodeValue& value)
  {
    values_.append(value);
  }

  void Push(const NodeParam::DataType& type, const QVariant& data, const Node *from, const QString& tag = QString())
  {
    Push(NodeValue(type, data, from, tag));
  }

  void Push(const ShaderValue &value, const Node *from)
  {
    Push(value.type, value.data, from, value.tag);
  }

  void Prepend(const NodeValue& value)
  {
    values_.prepend(value);
  }

  void Prepend(const NodeParam::DataType& type, const QVariant& data, const Node *from, const QString& tag = QString())
  {
    Prepend(NodeValue(type, data, from, tag));
  }

  void Prepend(const ShaderValue &value, const Node *from)
  {
    Prepend(value.type, value.data, from, value.tag);
  }

  const NodeValue& at(int index) const
  {
    return values_.at(index);
  }
  NodeValue TakeAt(int index)
  {
    return values_.takeAt(index);
  }

  int Count() const
  {
    return values_.size();
  }

  bool Has(const NodeParam::DataType& type) const;
  void Remove(const NodeValue& v);

  bool isEmpty() const
  {
    return values_.isEmpty();
  }

  static NodeValueTable Merge(QList<NodeValueTable> tables);

private:
  int GetInternal(const NodeParam::DataType& type, const QString& tag) const;

  QList<NodeValue> values_;

};

class NodeValueDatabase
{
public:
  NodeValueDatabase() = default;

  NodeValueTable& operator[](const QString& input_id)
  {
    return tables_[input_id];
  }

  NodeValueTable& operator[](const NodeInput* input)
  {
    return tables_[input->id()];
  }

  void Insert(const QString& key, const NodeValueTable &value)
  {
    tables_.insert(key, value);
  }

  void Insert(const NodeInput* key, const NodeValueTable& value)
  {
    tables_.insert(key->id(), value);
  }

  NodeValueTable Merge() const;

  using const_iterator = QHash<QString, NodeValueTable>::const_iterator;

  inline QHash<QString, NodeValueTable>::const_iterator begin() const
  {
    return tables_.cbegin();
  }

  inline QHash<QString, NodeValueTable>::const_iterator end() const
  {
    return tables_.cend();
  }

  inline bool contains(const QString& s) const
  {
    return tables_.contains(s);
  }

private:
  QHash<QString, NodeValueTable> tables_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeValue)
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeValueTable)
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeValueDatabase)

#endif // VALUE_H

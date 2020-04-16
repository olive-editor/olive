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

OLIVE_NAMESPACE_ENTER

class NodeValue
{
public:
  NodeValue() = default;
  NodeValue(const NodeParam::DataType& type, const QVariant& data, const QString& tag = QString());

  const NodeParam::DataType& type() const;
  const QVariant& data() const;
  const QString& tag() const;

  bool operator==(const NodeValue& rhs) const;

private:
  NodeParam::DataType type_;
  QVariant data_;
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
  void Push(const NodeValue& value);
  void Push(const NodeParam::DataType& type, const QVariant& data, const QString& tag = QString());
  void Prepend(const NodeValue& value);
  void Prepend(const NodeParam::DataType& type, const QVariant& data, const QString& tag = QString());
  const NodeValue& At(int index) const;
  NodeValue TakeAt(int index);
  int Count() const;
  bool Has(const NodeParam::DataType& type) const;
  void Remove(const NodeValue& v);

  bool isEmpty() const;

  static NodeValueTable Merge(QList<NodeValueTable> tables);

private:
  int GetInternal(const NodeParam::DataType& type, const QString& tag) const;

  QList<NodeValue> values_;

};

class NodeValueDatabase
{
public:
  NodeValueDatabase() = default;

  NodeValueTable& operator[](const QString& input_id);
  NodeValueTable& operator[](const NodeInput* input);

  const NodeValueTable operator[](const QString& input_id) const;
  const NodeValueTable operator[](const NodeInput* input) const;

  void Insert(const QString& key, const NodeValueTable &value);
  void Insert(const NodeInput* key, const NodeValueTable& value);

  NodeValueTable Merge() const;

private:
  QHash<QString, NodeValueTable> tables_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeValueTable)
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::NodeValueDatabase)

#endif // VALUE_H

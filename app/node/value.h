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

#ifndef VALUE_H
#define VALUE_H

#include <QString>
#include <QVariant>

namespace olive {

class Node;
class NodeInput;

class NodeValue
{
public:
  /**
   * @brief The types of data that can be passed between Nodes
   */
  enum Type {
    kNone,

    /**
     ****************************** SPECIFIC IDENTIFIERS ******************************
     */

    /**
     * Integer type
     *
     * Resolves to int64_t.
     */
    kInt,

    /**
     * Decimal (floating-point) type
     *
     * Resolves to `double`.
     */
    kFloat,

    /**
     * Decimal (rational) type
     *
     * Resolves to `double`.
     */
    kRational,

    /**
     * Boolean type
     *
     * Resolves to `bool`.
     */
    kBoolean,

    /**
     * Floating-point type
     *
     * Resolves to `Color`.
     *
     * Colors passed around the nodes should always be in reference space and preferably use
     */
    kColor,

    /**
     * Matrix type
     *
     * Resolves to `QMatrix4x4`.
     */
    kMatrix,

    /**
     * Text type
     *
     * Resolves to `QString`.
     */
    kText,

    /**
     * Font type
     *
     * Resolves to `QFont`.
     */
    kFont,

    /**
     * File type
     *
     * Resolves to a `QString` containing an absolute file path.
     */
    kFile,

    /**
     * Image buffer type
     *
     * True value type depends on the render engine used.
     */
    kTexture,

    /**
     * Audio samples type
     *
     * Resolves to `SampleBufferPtr`.
     */
    kSamples,

    /**
     * Footage stream identifier type
     *
     * Resolves to `StreamPtr`.
     */
    kFootage,

    /**
     * Two-dimensional vector (XY) type
     *
     * Resolves to `QVector2D`.
     */
    kVec2,

    /**
     * Three-dimensional vector (XYZ) type
     *
     * Resolves to `QVector3D`.
     */
    kVec3,

    /**
     * Four-dimensional vector (XYZW) type
     *
     * Resolves to `QVector4D`.
     */
    kVec4,

    /**
     * ComboBox type
     *
     * Resolves to `int` - the index currently selected
     */
    kCombo,

    /**
     * Job type
     *
     * An internal type used to indicate to the renderer that an accelerated shader job needs to
     * run. This value will usually be taken from a table and a kTexture value will be pushed to
     * take its place.
     */
    kShaderJob,

    /**
     * Job type
     *
     * An internal type used to indicate to the renderer that an accelerated sample job needs to
     * take place. This value will usually be taken from a table and a kSamples value will be
     * pushed to take its place.
     */
    kSampleJob,

    /**
     * Job type
     *
     * An internal type used to indicate to the renderer that an accelerated sample job needs to
     * take place. This value will usually be taken from a table and a kSamples value will be
     * pushed to take its place.
     */
    kGenerateJob
  };

  static const QVector<Type> kNumber;
  static const QVector<Type> kBuffer;
  static const QVector<Type> kVector;

  NodeValue() :
    type_(kNone),
    from_(nullptr),
    array_(false)
  {
  }

  NodeValue(Type type, const QVariant& data, const Node* from = nullptr, bool array = false, const QString& tag = QString()) :
    type_(type),
    data_(data),
    from_(from),
    tag_(tag),
    array_(array)
  {
  }

  Type type() const
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

  bool array() const
  {
    return array_;
  }

  bool operator==(const NodeValue& rhs) const
  {
    return type_ == rhs.type_ && tag_ == rhs.tag_ && data_ == rhs.data_;
  }

  static QString GetPrettyDataTypeName(Type type);

  static QString ValueToString(Type data_type, const QVariant& value, bool value_is_a_key_track);

  static QVariant StringToValue(Type data_type, const QString &string, bool value_is_a_key_track);

  /**
   * @brief Convert a value from a NodeParam into bytes
   */
  static QByteArray ValueToBytes(Type type, const QVariant& value);

  static QVector<QVariant> split_normal_value_into_track_values(Type type, const QVariant &value);

  static QVariant combine_track_values_into_normal_value(Type type, const QVector<QVariant>& split);

  /**
   * @brief Returns whether a data type can be interpolated or not
   */
  static bool type_can_be_interpolated(NodeValue::Type type)
  {
    return type == kFloat
        || type == kVec2
        || type == kVec3
        || type == kVec4
        || type == kColor;
  }

  static bool type_is_numeric(NodeValue::Type type)
  {
    return kNumber.contains(type);
  }

  static bool type_is_vector(NodeValue::Type type)
  {
    return kVector.contains(type);
  }

  static int get_number_of_keyframe_tracks(Type type);

  static void ValidateVectorString(QStringList* list, int count);

private:
  Type type_;
  QVariant data_;
  const Node* from_;
  QString tag_;
  bool array_;

};

class NodeValueTable
{
public:
  NodeValueTable() = default;

  QVariant Get(NodeValue::Type type, const QString& tag = QString()) const
  {
    QVector<NodeValue::Type> types = {type};
    return Get(types, tag);
  }

  QVariant Get(const QVector<NodeValue::Type>& type, const QString& tag = QString()) const
  {
    return GetWithMeta(type, tag).data();
  }

  NodeValue GetWithMeta(NodeValue::Type type, const QString& tag = QString()) const
  {
    QVector<NodeValue::Type> types = {type};
    return GetWithMeta(types, tag);
  }

  NodeValue GetWithMeta(const QVector<NodeValue::Type>& type, const QString& tag = QString()) const;

  QVariant Take(NodeValue::Type type, const QString& tag = QString())
  {
    QVector<NodeValue::Type> types = {type};
    return Take(types, tag);
  }

  QVariant Take(const QVector<NodeValue::Type>& type, const QString& tag = QString())
  {
    return TakeWithMeta(type, tag).data();
  }

  NodeValue TakeWithMeta(NodeValue::Type type, const QString& tag = QString())
  {
    QVector<NodeValue::Type> types = {type};
    return TakeWithMeta(types, tag);
  }

  NodeValue TakeWithMeta(const QVector<NodeValue::Type>& type, const QString& tag = QString());

  void Push(const NodeValue& value)
  {
    values_.append(value);
  }

  void Push(NodeValue::Type type, const QVariant& data, const Node *from, bool array = false, const QString& tag = QString())
  {
    Push(NodeValue(type, data, from, array, tag));
  }

  void Prepend(const NodeValue& value)
  {
    values_.prepend(value);
  }

  void Prepend(NodeValue::Type type, const QVariant& data, const Node *from, bool array = false, const QString& tag = QString())
  {
    Prepend(NodeValue(type, data, from, array, tag));
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

  bool Has(NodeValue::Type type) const;
  void Remove(const NodeValue& v);

  bool isEmpty() const
  {
    return values_.isEmpty();
  }

  static NodeValueTable Merge(QList<NodeValueTable> tables);

private:
  int GetInternal(const QVector<NodeValue::Type> &type, const QString& tag) const;

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

  NodeValueTable& operator[](const NodeInput* input);

  void Insert(const QString& key, const NodeValueTable &value)
  {
    tables_.insert(key, value);
  }

  void Insert(const NodeInput* key, const NodeValueTable& value);

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

using NodeValueMap = QHash<QString, NodeValue>;

}

Q_DECLARE_METATYPE(olive::NodeValue)
Q_DECLARE_METATYPE(olive::NodeValueTable)
Q_DECLARE_METATYPE(olive::NodeValueDatabase)

#endif // VALUE_H

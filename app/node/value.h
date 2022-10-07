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

#ifndef NODEVALUE_H
#define NODEVALUE_H

#include <QMatrix4x4>
#include <QString>
#include <QVariant>
#include <QVector>

#include "codec/samplebuffer.h"
#include "common/bezier.h"
#include "node/splitvalue.h"
#include "render/color.h"
#include "render/texture.h"

namespace olive {

class Node;
class NodeValue;
class NodeValueTable;

using NodeValueArray = std::map<int, NodeValue>;
using NodeValueTableArray = std::map<int, NodeValueTable>;

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
     * Cubic bezier type that contains three X/Y coordinates, the main point, and two control points
     *
     * Resolves to `Bezier`
     */
    kBezier,

    /**
     * ComboBox type
     *
     * Resolves to `int` - the index currently selected
     */
    kCombo,

    /**
     * Video Parameters type
     *
     * Resolves to `VideoParams`
     */
    kVideoParams,

    /**
     * Audio Parameters type
     *
     * Resolves to `AudioParams`
     */
    kAudioParams,

    /**
     * Subtitle Parameters type
     *
     * Resolves to `SubtitleParams`
     */
    kSubtitleParams,

    /**
     * End of list
     */
    kDataTypeCount
  };

  NodeValue() :
    type_(kNone),
    from_(nullptr),
    array_(false)
  {
  }

  template <typename T>
  NodeValue(Type type, const T& data, const Node* from = nullptr, bool array = false, const QString& tag = QString()) :
    type_(type),
    from_(from),
    tag_(tag),
    array_(array)
  {
    set_value(data);
  }

  template <typename T>
  NodeValue(Type type, const T& data, const Node* from, const QString& tag) :
    NodeValue(type, data, from, false, tag)
  {
  }

  Type type() const
  {
    return type_;
  }

  template <typename T>
  T value() const
  {
    return data_.value<T>();
  }

  template <typename T>
  void set_value(const T &v)
  {
    data_ = QVariant::fromValue(v);
  }

  const QVariant &data() const { return data_; }

  template <typename T>
  bool canConvert() const
  {
    return data_.canConvert<T>();
  }

  const QString& tag() const
  {
    return tag_;
  }

  void set_tag(const QString& tag)
  {
    tag_ = tag;
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

  operator bool() const
  {
    return !data_.isNull();
  }

  static QString GetPrettyDataTypeName(Type type);

  static QString GetDataTypeName(Type type);
  static NodeValue::Type GetDataTypeFromName(const QString &n);

  static QString ValueToString(Type data_type, const QVariant& value, bool value_is_a_key_track);
  static QString ValueToString(const NodeValue &v, bool value_is_a_key_track)
  {
    return ValueToString(v.type_, v.data_, value_is_a_key_track);
  }

  static QVariant StringToValue(Type data_type, const QString &string, bool value_is_a_key_track);

  static QVector<QVariant> split_normal_value_into_track_values(Type type, const QVariant &value);

  static QVariant combine_track_values_into_normal_value(Type type, const QVector<QVariant>& split);

  SplitValue to_split_value() const
  {
    return split_normal_value_into_track_values(type_, data_);
  }

  /**
   * @brief Returns whether a data type can be interpolated or not
   */
  static bool type_can_be_interpolated(NodeValue::Type type)
  {
    return type == kFloat
        || type == kVec2
        || type == kVec3
        || type == kVec4
        || type == kBezier
        || type == kColor
        || type == kRational;
  }

  static bool type_is_numeric(NodeValue::Type type)
  {
    return type == kFloat
        || type == kInt
        || type == kRational;
  }

  static bool type_is_vector(NodeValue::Type type)
  {
    return type == kVec2
        || type == kVec3
        || type == kVec4;
  }

  static bool type_is_buffer(NodeValue::Type type)
  {
    return type == kTexture
        || type == kSamples;
  }

  static int get_number_of_keyframe_tracks(Type type);

  static void ValidateVectorString(QStringList* list, int count);

  TexturePtr toTexture() const { return value<TexturePtr>(); }
  SampleBuffer toSamples() const { return value<SampleBuffer>(); }
  bool toBool() const { return value<bool>(); }
  double toDouble() const { return value<double>(); }
  int64_t toInt() const { return value<int64_t>(); }
  rational toRational() const { return value<rational>(); }
  QString toString() const { return value<QString>(); }
  Color toColor() const { return value<Color>(); }
  QMatrix4x4 toMatrix() const { return value<QMatrix4x4>(); }
  VideoParams toVideoParams() const { return value<VideoParams>(); }
  AudioParams toAudioParams() const { return value<AudioParams>(); }
  QVector2D toVec2() const { return value<QVector2D>(); }
  QVector3D toVec3() const { return value<QVector3D>(); }
  QVector4D toVec4() const { return value<QVector4D>(); }
  Bezier toBezier() const { return value<Bezier>(); }
  NodeValueArray toArray() const { return value<NodeValueArray>(); }

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

  NodeValue Get(NodeValue::Type type, const QString& tag = QString()) const
  {
    QVector<NodeValue::Type> types = {type};
    return Get(types, tag);
  }

  NodeValue Get(const QVector<NodeValue::Type>& type, const QString& tag = QString()) const;

  NodeValue Take(NodeValue::Type type, const QString& tag = QString())
  {
    QVector<NodeValue::Type> types = {type};
    return Take(types, tag);
  }

  NodeValue Take(const QVector<NodeValue::Type>& type, const QString& tag = QString());

  void Push(const NodeValue& value)
  {
    values_.append(value);
  }

  void Push(const NodeValueTable& value)
  {
    values_.append(value.values_);
  }

  template <typename T>
  void Push(NodeValue::Type type, const T& data, const Node *from, bool array = false, const QString& tag = QString())
  {
    Push(NodeValue(type, data, from, array, tag));
  }

  template <typename T>
  void Push(NodeValue::Type type, const T& data, const Node *from, const QString& tag)
  {
    Push(NodeValue(type, data, from, false, tag));
  }

  void Prepend(const NodeValue& value)
  {
    values_.prepend(value);
  }

  template <typename T>
  void Prepend(NodeValue::Type type, const T& data, const Node *from, bool array = false, const QString& tag = QString())
  {
    Prepend(NodeValue(type, data, from, array, tag));
  }

  template <typename T>
  void Prepend(NodeValue::Type type, const T& data, const Node *from, const QString& tag)
  {
    Prepend(NodeValue(type, data, from, false, tag));
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

  void Clear()
  {
    values_.clear();
  }

  bool isEmpty() const
  {
    return values_.isEmpty();
  }

  int GetValueIndex(const QVector<NodeValue::Type> &type, const QString& tag) const;

  static NodeValueTable Merge(QList<NodeValueTable> tables);

private:
  QVector<NodeValue> values_;

};

using NodeValueRow = QHash<QString, NodeValue>;

}

Q_DECLARE_METATYPE(olive::NodeValue)
Q_DECLARE_METATYPE(olive::NodeValueTable)

#endif // NODEVALUE_H
